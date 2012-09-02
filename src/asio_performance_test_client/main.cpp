//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <ma/config.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/async_connect.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/limited_int.hpp>

#if defined(MA_HAS_BOOST_TIMER)
#include <boost/timer/timer.hpp>
#endif // defined(MA_HAS_BOOST_TIMER)

namespace {

class work_state : private boost::noncopyable
{
public:
  explicit work_state(std::size_t outstanding)
    : outstanding_(outstanding)
  {
  }

  void dec_outstanding()
  {
    boost::mutex::scoped_lock lock(mutex_);
    --outstanding_;
    if (!outstanding_)
    {
      condition_.notify_all();
    }
  }

  void wait(const boost::posix_time::time_duration& timeout)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (outstanding_)
    {
      condition_.timed_wait(lock, timeout);
    }
  }

private:
  std::size_t outstanding_;
  boost::mutex mutex_;
  boost::condition_variable condition_;
}; // struct work_state

typedef ma::limited_int<boost::uintmax_t> limited_counter;

template <typename Integer>
std::string to_string(const ma::limited_int<Integer>& limited_value)
{
  if (limited_value.overflowed())
  {
    return ">" + boost::lexical_cast<std::string>(limited_value.value());
  }
  else
  {
    return boost::lexical_cast<std::string>(limited_value.value());
  }
}

class stats : private boost::noncopyable
{
public:
  stats()
    : total_sessions_connected_()
    , total_bytes_written_()
    , total_bytes_read_()
  {
  }

  void add(const limited_counter& bytes_written,
      const limited_counter& bytes_read)
  {
    ++total_sessions_connected_;
    total_bytes_written_ += bytes_written;
    total_bytes_read_    += bytes_read;
  }

  void print()
  {
    std::cout << "Total sessions connected: "
              << to_string(total_sessions_connected_)
              << std::endl
              << "Total bytes written     : "
              << to_string(total_bytes_written_)
              << std::endl
              << "Total bytes read        : "
              << to_string(total_bytes_read_)
              << std::endl;
  }

private:
  limited_counter total_sessions_connected_;
  limited_counter total_bytes_written_;
  limited_counter total_bytes_read_;
}; // class stats

typedef boost::optional<bool> optional_bool;
typedef boost::optional<int>  optional_int;

struct session_config
{
public:
  session_config(std::size_t the_buffer_size,
      std::size_t the_max_connect_attempts,
      const optional_int& the_socket_recv_buffer_size,
      const optional_int& the_socket_send_buffer_size,
      const optional_bool& the_no_delay)
    : buffer_size(the_buffer_size)
    , max_connect_attempts(the_max_connect_attempts)
    , socket_recv_buffer_size(the_socket_recv_buffer_size)
    , socket_send_buffer_size(the_socket_send_buffer_size)
    , no_delay(the_no_delay)
  {
    BOOST_ASSERT_MSG(the_buffer_size > 0, "buffer_size must be > 0");

    BOOST_ASSERT_MSG(
        !the_socket_recv_buffer_size || (*the_socket_recv_buffer_size) >= 0,
        "Defined socket_recv_buffer_size must be >= 0");

    BOOST_ASSERT_MSG(
        !the_socket_send_buffer_size || (*the_socket_send_buffer_size) >= 0,
        "Defined socket_send_buffer_size must be >= 0");
  }

  std::size_t   buffer_size;
  std::size_t   max_connect_attempts;
  optional_int  socket_recv_buffer_size;
  optional_int  socket_send_buffer_size;
  optional_bool no_delay;
}; // struct session_config

class session : private boost::noncopyable
{
  typedef session this_type;

public:
  typedef boost::asio::ip::tcp protocol;

  session(boost::asio::io_service& io_service, const session_config& config,
      work_state& work_state)
    : max_connect_attempts_(config.max_connect_attempts)
    , socket_recv_buffer_size_(config.socket_recv_buffer_size)
    , socket_send_buffer_size_(config.socket_send_buffer_size)
    , no_delay_(config.no_delay)
    , strand_(io_service)
    , socket_(io_service)
    , buffer_(config.buffer_size)
    , bytes_written_()
    , bytes_read_()
    , connected_(false)
    , write_in_progress_(false)
    , read_in_progress_(false)
    , stopped_(false)
    , was_connected_(false)
    , work_state_(work_state)
  {
    typedef ma::cyclic_buffer::mutable_buffers_type buffers_type;
    std::size_t filled_size = config.buffer_size / 2;
    const buffers_type data = buffer_.prepared();
    std::size_t size_to_fill = filled_size;
    for (buffers_type::const_iterator i = data.begin(), end = data.end();
        size_to_fill && (i != end); ++i)
    {
      char* b = boost::asio::buffer_cast<char*>(*i);
      std::size_t s = boost::asio::buffer_size(*i);
      for (; size_to_fill && s; ++b, --size_to_fill, --s)
      {
        *b = static_cast<char>(config.buffer_size % 128);
      }
    }
    buffer_.consume(filled_size);
  }

  ~session()
  {
    BOOST_ASSERT_MSG(!connected_, "Invalid connect state");
    BOOST_ASSERT_MSG(!read_in_progress_, "Invalid read state");
    BOOST_ASSERT_MSG(!write_in_progress_, "Invalid write state");
    BOOST_ASSERT_MSG(stopped_, "Session was not stopped");
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(write_allocator_,
        boost::bind(&this_type::do_start, this, 0, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  bool was_connected() const
  {
    return was_connected_;
  }

  limited_counter bytes_written() const
  {
    return bytes_written_;
  }

  limited_counter bytes_read() const
  {
    return bytes_read_;
  }

private:
  void do_start(std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    start_connect(connect_attempt,
        initial_endpoint_iterator, initial_endpoint_iterator);
  }

  void do_stop()
  {
    if (stopped_)
    {
      return;
    }

    if (connected_)
    {
      shutdown_socket();
    }
    stop();
  }

  void start_connect(std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator,
      const protocol::resolver::iterator& current_endpoint_iterator)
  {
    protocol::endpoint endpoint = *current_endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(write_allocator_,
            boost::bind(&this_type::handle_connect, this, _1, connect_attempt,
                initial_endpoint_iterator, current_endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& error,
      std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator,
      protocol::resolver::iterator current_endpoint_iterator)
  {
    // Collect statistics at first step
    was_connected_ = !error;

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      close_socket();

      ++current_endpoint_iterator;
      if (protocol::resolver::iterator() != current_endpoint_iterator)
      {
        start_connect(connect_attempt,
            initial_endpoint_iterator, current_endpoint_iterator);
        return;
      }

      if (max_connect_attempts_)
      {
        ++connect_attempt;
        if (connect_attempt >= max_connect_attempts_)
        {
          stop();
          return;
        }
      }

      start_connect(connect_attempt,
          initial_endpoint_iterator, initial_endpoint_iterator);
      return;
    }

    connected_ = true;

    if (boost::system::error_code error = apply_socket_options())
    {
      stop();
      return;
    }

    start_write_some();
    start_read_some();
  }

  void handle_read(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    read_in_progress_ = false;

    // Collect statistics at first step
    bytes_read_ += bytes_transferred;
    buffer_.consume(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (boost::asio::error::eof == error)
    {
      shutdown_socket();
      stop();
      return;
    }

    if (error)
    {
      stop();
      return;
    }

    if (!write_in_progress_)
    {
      start_write_some();
    }
    start_read_some();
  }

  void handle_write(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    write_in_progress_ = false;

    // Collect statistics at first step
    bytes_written_ += bytes_transferred;
    buffer_.commit(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      stop();
      return;
    }

    if (!read_in_progress_)
    {
      start_read_some();
    }
    start_write_some();
  }

  void stop()
  {
    close_socket();
    connected_ = false;
    stopped_   = true;
    work_state_.dec_outstanding();
  }

  void start_write_some()
  {
    ma::cyclic_buffer::const_buffers_type write_data = buffer_.data();
    if (!write_data.empty())
    {
      socket_.async_write_some(write_data, MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(write_allocator_,
              boost::bind(&this_type::handle_write, this, _1, _2))));
      write_in_progress_ = true;
    }
  }

  void start_read_some()
  {
    ma::cyclic_buffer::mutable_buffers_type read_data = buffer_.prepared();
    if (!read_data.empty())
    {
      socket_.async_read_some(read_data, MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(read_allocator_,
              boost::bind(&this_type::handle_read, this, _1, _2))));
      read_in_progress_ = true;
    }
  }

  boost::system::error_code apply_socket_options()
  {
    typedef protocol::socket socket_type;

    // Setup abortive shutdown sequence for closesocket
    {
      boost::system::error_code error;
      socket_type::linger opt(false, 0);
      socket_.set_option(opt, error);
      if (error)
      {
        return error;
      }
    }

    // Apply all (really) configered socket options
    if (socket_recv_buffer_size_)
    {
      boost::system::error_code error;
      socket_type::receive_buffer_size opt(*socket_recv_buffer_size_);
      socket_.set_option(opt, error);
      if (error)
      {
        return error;
      }
    }

    if (socket_send_buffer_size_)
    {
      boost::system::error_code error;
      socket_type::send_buffer_size opt(*socket_send_buffer_size_);
      socket_.set_option(opt, error);
      if (error)
      {
        return error;
      }
    }

    if (no_delay_)
    {
      boost::system::error_code error;
      socket_type::linger opt(*no_delay_, 0);
      socket_.set_option(opt, error);
      if (error)
      {
        return error;
      }
    }

    return boost::system::error_code();
  }

  boost::system::error_code shutdown_socket()
  {
    boost::system::error_code error;
    socket_.shutdown(protocol::socket::shutdown_send, error);
    return error;
  }

  void close_socket()
  {
    boost::system::error_code ignored;
    socket_.close(ignored);
  }

  const std::size_t   max_connect_attempts_;
  const optional_int  socket_recv_buffer_size_;
  const optional_int  socket_send_buffer_size_;
  const optional_bool no_delay_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  ma::cyclic_buffer buffer_;
  limited_counter bytes_written_;
  limited_counter bytes_read_;
  bool connected_;
  bool write_in_progress_;
  bool read_in_progress_;
  bool stopped_;
  bool was_connected_;
  work_state& work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
}; // class session

typedef boost::shared_ptr<session>     session_ptr;
typedef ma::steady_deadline_timer      deadline_timer;
typedef deadline_timer::duration_type  duration_type;
typedef boost::optional<duration_type> optional_duration;

struct session_manager_config
{
public:
  session_manager_config(std::size_t the_session_count,
      std::size_t the_block_size,
      const optional_duration the_block_pause,
      const session_config& the_managed_session_config)
    : session_count(the_session_count)
    , block_size(the_block_size)
    , block_pause(the_block_pause)
    , managed_session_config(the_managed_session_config)
  {
  }

  std::size_t       session_count;
  std::size_t       block_size;
  optional_duration block_pause;
  session_config    managed_session_config;
}; // struct session_manager_config

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::vector<io_service_ptr> io_service_vector;
typedef boost::shared_ptr<boost::asio::io_service::work> io_service_work_ptr;
typedef std::vector<io_service_work_ptr> io_service_work_vector;

class session_manager : private boost::noncopyable
{
private:
  typedef session_manager this_type;

public:
  typedef session::protocol protocol;

  session_manager(boost::asio::io_service& session_manager_io_service,
      const io_service_vector& session_io_services,
      const session_manager_config& config)
    : block_size_(config.block_size)
    , block_pause_(config.block_pause)
    , io_service_(session_manager_io_service)
    , strand_(session_manager_io_service)
    , timer_(session_manager_io_service)
    , sessions_()
    , stopped_(false)
    , timer_in_progess_(false)
    , stats_()
    , work_state_(config.session_count)
  {
    typedef io_service_vector::const_iterator iterator;

    const iterator sbegin = session_io_services.begin();
    const iterator send   = session_io_services.end();
    for (std::size_t i = 0; i != config.session_count;)
    {
      for (iterator j = sbegin; (j != send) && (i != config.session_count);
          ++j, ++i)
      {
        sessions_.push_back(boost::make_shared<session>(boost::ref(**j),
            config.managed_session_config, boost::ref(work_state_)));
      }
    }
  }

  ~session_manager()
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "Invalid timer state");

    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&this_type::register_stats, this, _1));
    stats_.print();
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(start_allocator_,
        boost::bind(&this_type::do_start, this, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  void wait(const boost::posix_time::time_duration& timeout)
  {
    work_state_.wait(timeout);
  }

private:
  typedef std::vector<session_ptr> session_vector;

  std::size_t start_block(
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset, std::size_t max_block_size)
  {
    std::size_t block_size = 0;
    for (session_vector::const_iterator i = sessions_.begin() + offset,
        end = sessions_.end(); (i != end) && (block_size != max_block_size);
        ++i, ++block_size)
    {
      (*i)->async_start(endpoint_iterator);
    }
    return offset + block_size;
  }

  void do_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    if (block_pause_)
    {
      std::size_t offset = start_block(endpoint_iterator, 0, block_size_);
      if (offset < sessions_.size())
      {
        start_deferred_session_start(endpoint_iterator, offset);
      }
    }
    else
    {
      start_block(endpoint_iterator, 0, sessions_.size());
    }
  }

  void start_deferred_session_start(
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset)
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "Invalid timer state");

    timer_.expires_from_now(*block_pause_);
    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_,
            boost::bind(&this_type::handle_timer, this,
                _1, endpoint_iterator, offset))));
    timer_in_progess_ = true;
  }

  void handle_timer(const boost::system::error_code& error,
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset)
  {
    timer_in_progess_ = false;

    if (stopped_)
    {
      return;
    }

    if (error && error != boost::asio::error::operation_aborted)
    {
      do_stop();
      return;
    }

    offset = start_block(endpoint_iterator, offset, block_size_);
    if (offset < sessions_.size())
    {
      start_deferred_session_start(endpoint_iterator, offset);
    }
  }

  void cancel_timer()
  {
    if (timer_in_progess_)
    {
      boost::system::error_code ignored;
      timer_.cancel(ignored);
    }
  }

  void do_stop()
  {
    if (stopped_)
    {
      return;
    }

    cancel_timer();
    stopped_ = true;
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&session::async_stop, _1));
  }

  void register_stats(const session_ptr& session)
  {
    if (session->was_connected())
    {
      stats_.add(session->bytes_written(), session->bytes_read());
    }
  }

  const std::size_t        block_size_;
  const optional_duration  block_pause_;
  boost::asio::io_service& io_service_;
  boost::asio::io_service::strand strand_;
  deadline_timer timer_;
  session_vector sessions_;
  bool  stopped_;
  bool  timer_in_progess_;
  stats stats_;
  work_state work_state_;
  ma::in_place_handler_allocator<256> start_allocator_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> timer_allocator_;
}; // class session_manager

optional_duration to_optional_duration(long milliseconds)
{
  optional_duration duration;
  if (milliseconds)
  {
    duration = ma::to_steady_deadline_timer_duration(
        boost::posix_time::milliseconds(milliseconds));
  }
  return duration;
}

struct client_config
{
public:
  client_config(bool the_ios_per_work_thread,
      const std::string& the_host,
      const std::string& the_port,
      std::size_t the_thread_count,
      const boost::posix_time::time_duration& the_test_duration,
      const session_manager_config& the_session_manager_config)
    : ios_per_work_thread(the_ios_per_work_thread)
    , host(the_host)
    , port(the_port)
    , thread_count(the_thread_count)
    , test_duration(the_test_duration)
    , client_session_manager_config(the_session_manager_config)
  {
  }

  bool        ios_per_work_thread;
  std::string host;
  std::string port;
  std::size_t thread_count;
  boost::posix_time::time_duration test_duration;
  session_manager_config client_session_manager_config;
}; // struct client_config

const char* help_option_name                    = "help";
const char* host_option_name                    = "host";
const char* port_option_name                    = "port";
const char* demux_option_name                   = "demux_per_work_thread";
const char* threads_option_name                 = "threads";
const char* sessions_option_name                = "sessions";
const char* block_size_option_name              = "block_size";
const char* block_pause_option_name             = "block_pause";
const char* buffer_option_name                  = "buffer";
const char* connect_attempts_option_name        = "connect_attempts";
const char* socket_recv_buffer_size_option_name = "sock_recv_buffer";
const char* socket_send_buffer_size_option_name = "sock_send_buffer";
const char* no_delay_option_name                = "no_delay";
const char* time_option_name                    = "time";
const std::string default_system_value          = "system default";

std::size_t calc_thread_count(std::size_t hardware_concurrency)
{
  if (hardware_concurrency)
  {
    return hardware_concurrency;
  }
  return 2;
}

boost::program_options::options_description build_cmd_options_description(
    std::size_t hardware_concurrency)
{
#if defined(WIN32)
  bool default_ios_per_work_thread = false;
#else
  bool default_ios_per_work_thread = true;
#endif

  boost::program_options::options_description description("Allowed options");

  description.add_options()
    (
      help_option_name,
      "produce help message"
    )
    (
      host_option_name,
      boost::program_options::value<std::string>(),
      "set the remote peer's host address"
    )
    (
      port_option_name,
      boost::program_options::value<std::string>(),
      "set the remote peer's port"
    )
    (
      demux_option_name,
      boost::program_options::value<bool>()->default_value(
          default_ios_per_work_thread),
      "set demultiplexer-per-work-thread mode on"
    )
    (
      threads_option_name,
      boost::program_options::value<std::size_t>()->default_value(
          calc_thread_count(hardware_concurrency)),
      "set the number of threads"
    )
    (
      sessions_option_name,
      boost::program_options::value<std::size_t>()->default_value(10000),
      "set the maximum number of simultaneous TCP connections"
    )
    (
      block_size_option_name,
      boost::program_options::value<std::size_t>()->default_value(100),
      "set the number of simultaneous running connect operations"
    )
    (
      block_pause_option_name,
      boost::program_options::value<long>()->default_value(1000),
      "set the pause betweeen simultaneous running" \
          "  connect operations (milliseconds)"
    )
    (
      buffer_option_name,
      boost::program_options::value<std::size_t>()->default_value(4096),
      "set the size of session's buffer (bytes)"
    )
    (
      connect_attempts_option_name,
      boost::program_options::value<std::size_t>()->default_value(0),
      "set the maximum number of attempts to connect to remote peer" \
          ", 0 means infinity"
    )
    (
      socket_recv_buffer_size_option_name,
      boost::program_options::value<int>(),
      "set the size of session's socket receive buffer (bytes)"
    )
    (
      socket_send_buffer_size_option_name,
      boost::program_options::value<int>(),
      "set the size of session's socket send buffer (bytes)"
    )
    (
      no_delay_option_name,
      boost::program_options::value<bool>(),
      "set TCP_NODELAY option of session's socket"
    )
    (
      time_option_name,
      boost::program_options::value<long>()->default_value(600),
      "set the duration of test (seconds)"
    );

  return description;
}

boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[])
{
  boost::program_options::variables_map values;
  boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), values);
  boost::program_options::notify(values);
  return values;
}

bool is_help_mode(const boost::program_options::variables_map& options_values)
{
  return 0 != options_values.count(help_option_name);
}

bool is_required_specified(
    const boost::program_options::variables_map& options_values)
{
  return (0 != options_values.count(port_option_name))
      && (0 != options_values.count(host_option_name));
}

optional_int build_optional_int(
    const boost::program_options::variables_map& options_values,
    const std::string& option_name)
{
  if (!options_values.count(option_name))
  {
    return optional_int();
  }
  return options_values[option_name].as<int>();
}

client_config build_client_config(
    const boost::program_options::variables_map& options_values)
{
  const std::string host = options_values[host_option_name].as<std::string>();
  const std::string port = options_values[port_option_name].as<std::string>();
  const std::size_t thread_count  =
      options_values[threads_option_name].as<std::size_t>();
  const long time_seconds =
      options_values[time_option_name].as<long>();
  const std::size_t session_count =
      options_values[sessions_option_name].as<std::size_t>();
  const std::size_t block_size =
      options_values[block_size_option_name].as<std::size_t>();
  const long block_pause_millis =
      options_values[block_pause_option_name].as<long>();

  const std::size_t buffer_size =
      options_values[buffer_option_name].as<std::size_t>();
  const std::size_t max_connect_attempts =
      options_values[connect_attempts_option_name].as<std::size_t>();

  const optional_int socket_recv_buffer_size =
      build_optional_int(options_values, socket_recv_buffer_size_option_name);
  const optional_int socket_send_buffer_size =
      build_optional_int(options_values, socket_send_buffer_size_option_name);

  optional_bool no_delay;
  if (0 != options_values.count(no_delay_option_name))
  {
    no_delay = options_values[no_delay_option_name].as<bool>();
  }

  session_config client_session_config(buffer_size, max_connect_attempts,
      socket_recv_buffer_size, socket_send_buffer_size, no_delay);

  session_manager_config client_session_manager_config(session_count,
      block_size, to_optional_duration(block_pause_millis),
      client_session_config);

  bool ios_per_work_thread =
      options_values[demux_option_name].as<bool>();

  return client_config(ios_per_work_thread, host, port, thread_count,
      boost::posix_time::seconds(time_seconds), client_session_manager_config);
}

std::string to_seconds_string(const boost::posix_time::time_duration& duration)
{
  return boost::lexical_cast<std::string>(duration.total_seconds());
}

std::string to_milliseconds_string(
    const boost::posix_time::time_duration& duration)
{
  return boost::lexical_cast<std::string>(duration.total_milliseconds());
}

#if defined(MA_HAS_STEADY_DEADLINE_TIMER)

std::string to_milliseconds_string(const duration_type& duration)
{
  return to_milliseconds_string(
      deadline_timer::traits_type::to_posix_duration(duration));
}

#endif // defined(MA_HAS_STEADY_DEADLINE_TIMER)

std::string to_milliseconds_string(const optional_duration& duration)
{
  if (duration)
  {
    return to_milliseconds_string(*duration);
  }
  else
  {
    return "n/a";
  }
}

std::string to_string(const optional_int& value)
{
  if (value)
  {
    return boost::lexical_cast<std::string>(*value);
  }
  else
  {
    return "n/a";
  }
}

std::string to_string(bool value)
{
  return value ? "on" : "off";
}

std::string to_string(const optional_bool& value)
{
  if (value)
  {
    return to_string(*value);
  }
  else
  {
    return "n/a";
  }
}

void print(const client_config& config)
{
  const session_manager_config& client_session_manager_config =
      config.client_session_manager_config;
  const session_config& managed_session_config =
      client_session_manager_config.managed_session_config;

  std::cout << "Host      : "
            << config.host
            << std::endl
            << "Port      : "
            << config.port
            << std::endl
            << "Threads   : "
            << config.thread_count
            << std::endl
            << "Sessions  : "
            << client_session_manager_config.session_count
            << std::endl
            << "Block size: "
            << client_session_manager_config.block_size
            << std::endl
            << "Block pause (milliseconds)        : "
            << to_milliseconds_string(
                  client_session_manager_config.block_pause)
            << std::endl
            << "Demultiplexer-per-work-thread mode: "
            << (to_string)(config.ios_per_work_thread)
            << std::endl
            << "Session's buffer size (bytes)     : "
            << managed_session_config.buffer_size
            << std::endl
            << "Maximum number of connect attempts per session : "
            << managed_session_config.max_connect_attempts
            << std::endl
            << "Size of session's socket receive buffer (bytes): "
            << (to_string)(managed_session_config.socket_recv_buffer_size)
            << std::endl
            << "Size of session's socket send buffer (bytes)   : "
            << (to_string)(managed_session_config.socket_send_buffer_size)
            << std::endl
            << "TCP_NODELAY   : "
            << (to_string)(managed_session_config.no_delay)
            << std::endl
            << "Time (seconds): "
            << to_seconds_string(config.test_duration)
            << std::endl;
}

io_service_vector create_session_io_services(const client_config& config)
{
  io_service_vector io_services;
  if (config.ios_per_work_thread)
  {
    for (std::size_t i = 0; i != config.thread_count; ++i)
    {
      io_services.push_back(boost::make_shared<boost::asio::io_service>(1));
    }
  }
  else
  {
    io_services.push_back(
        boost::make_shared<boost::asio::io_service>(config.thread_count));
  }
  return io_services;
}

io_service_work_vector create_works(const io_service_vector& io_services)
{
  io_service_work_vector works;
  for (io_service_vector::const_iterator i = io_services.begin(),
      end = io_services.end(); i != end; ++i)
  {
    works.push_back(
        boost::make_shared<boost::asio::io_service::work>(boost::ref(**i)));
  }
  return works;
}

void create_session_threads(boost::thread_group& threads,
    const client_config& config, const io_service_vector& io_services)
{
  if (config.ios_per_work_thread)
  {
    for (io_service_vector::const_iterator i = io_services.begin(),
        end = io_services.end(); i != end; ++i)
    {
      threads.create_thread(
          boost::bind(&boost::asio::io_service::run, i->get()));
    }
  }
  else
  {
    boost::asio::io_service& io_service = *io_services.front();
    for (std::size_t i = 0; i != config.thread_count; ++i)
    {
      threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service));
    }
  }
}

} // anonymous namespace

int main(int argc, char* argv[])
{
  try
  {
    const std::size_t cpu_count = boost::thread::hardware_concurrency();

    const boost::program_options::options_description
        cmd_options_description = build_cmd_options_description(cpu_count);

    // Parse user defined command line options
    const boost::program_options::variables_map cmd_options =
        parse_cmd_line(cmd_options_description, argc, argv);

    // Check work mode
    if (is_help_mode(cmd_options))
    {
      std::cout << cmd_options_description;
      return EXIT_SUCCESS;
    }

    // Check required options
    if (!is_required_specified(cmd_options))
    {
      std::cout << "Required options not specified" << std::endl
                << cmd_options_description;
      return EXIT_FAILURE;
    }

    const client_config config = build_client_config(cmd_options);
    print(config);

    const io_service_vector session_io_services =
        create_session_io_services(config);

    boost::asio::io_service session_manager_io_service(1);
    session_manager::protocol::resolver resolver(session_manager_io_service);
    session_manager client_session_manager(session_manager_io_service,
        session_io_services, config.client_session_manager_config);

    boost::thread_group session_threads;
    io_service_work_vector session_work_guards =
        create_works(session_io_services);
    create_session_threads(session_threads, config, session_io_services);

    boost::optional<boost::asio::io_service::work> session_manager_work_guard(
        boost::in_place(boost::ref(session_manager_io_service)));
    boost::thread session_manager_thread(boost::bind(
        &boost::asio::io_service::run, &session_manager_io_service));

#if defined(MA_HAS_BOOST_TIMER)
    boost::timer::cpu_timer timer;
#endif // defined(MA_HAS_BOOST_TIMER)

    client_session_manager.async_start(resolver.resolve(
        session_manager::protocol::resolver::query(config.host, config.port)));
    client_session_manager.wait(config.test_duration);
    client_session_manager.async_stop();

    session_manager_work_guard = boost::none;
    session_manager_thread.join();

    session_work_guards.clear();
    session_threads.join_all();

#if defined(MA_HAS_BOOST_TIMER)
    timer.stop();
    std::cout << "Test duration :" << timer.format();
#endif // defined(MA_HAS_BOOST_TIMER)

    return EXIT_SUCCESS;
  }
  catch (const boost::program_options::error& e)
  {
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return EXIT_FAILURE;
}
