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

#if defined(MA_HAS_BOOST_TIMER)
#include <boost/timer/timer.hpp>
#endif // defined(MA_HAS_BOOST_TIMER)

namespace {

class work_state : private boost::noncopyable
{
public:
  explicit work_state(std::size_t session_count)
    : session_count_(session_count)
  {
  }

  void notify_session_stop()
  {
    boost::mutex::scoped_lock lock(mutex_);
    --session_count_;
    if (!session_count_)
    {
      condition_.notify_all();
    }
  }

  void wait_for_all_session_stop(
      const boost::posix_time::time_duration& timeout)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (session_count_)
    {
      condition_.timed_wait(lock, timeout);
    }
  }

private:
  std::size_t session_count_;
  boost::mutex mutex_;
  boost::condition_variable condition_;
}; // struct work_state

class stats : private boost::noncopyable
{
public:
  stats()
    : total_sessions_connected_(0)
    , total_bytes_written_(0)
    , total_bytes_read_(0)
  {
  }

  void add(boost::uint_fast64_t bytes_written, boost::uint_fast64_t bytes_read)
  {
    ++total_sessions_connected_;
    total_bytes_written_ += bytes_written;
    total_bytes_read_ += bytes_read;
  }

  void print()
  {
    std::cout << "Total sessions connected: " << total_sessions_connected_
              << std::endl
              << "Total bytes written     : " << total_bytes_written_
              << std::endl
              << "Total bytes read        : " << total_bytes_read_
              << std::endl;
  }

private:
  std::size_t total_sessions_connected_;
  boost::uint_fast64_t total_bytes_written_;
  boost::uint_fast64_t total_bytes_read_;
}; // class stats

typedef boost::optional<bool> optional_bool;

struct session_config
{
public:
  session_config(std::size_t the_buffer_size,
      std::size_t the_max_connect_attempts,
      const optional_bool& the_no_delay)
    : buffer_size(the_buffer_size)
    , max_connect_attempts(the_max_connect_attempts)
    , no_delay(the_no_delay)
  {
  }

  std::size_t   buffer_size;
  std::size_t   max_connect_attempts;
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
    , no_delay_(config.no_delay)
    , strand_(io_service)
    , socket_(io_service)
    , buffer_(config.buffer_size)
    , bytes_written_(0)
    , bytes_read_(0)
    , was_connected_(false)
    , write_in_progress_(false)
    , read_in_progress_(false)
    , stopped_(false)
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

  boost::uint_fast64_t bytes_written() const
  {
    return bytes_written_;
  }

  boost::uint_fast64_t bytes_read() const
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
          do_stop();
          return;
        }
      }

      start_connect(connect_attempt,
          initial_endpoint_iterator, initial_endpoint_iterator);
      return;
    }

    was_connected_ = true;

    if (boost::system::error_code error = apply_socket_options())
    {
      do_stop();
      return;
    }

    start_write_some();
    start_read_some();
  }

  void handle_read(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    read_in_progress_ = false;
    bytes_read_ += bytes_transferred;
    buffer_.consume(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      do_stop();
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
    bytes_written_ += bytes_transferred;
    buffer_.commit(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      do_stop();
      return;
    }

    if (!read_in_progress_)
    {
      start_read_some();
    }
    start_write_some();
  }

  void do_stop()
  {
    if (stopped_)
    {
      return;
    }

    close_socket();
    stopped_ = true;
    work_state_.notify_session_stop();
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

  void close_socket()
  {
    boost::system::error_code ignored;
    socket_.close(ignored);
  }

  const std::size_t   max_connect_attempts_;
  const optional_bool no_delay_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  ma::cyclic_buffer buffer_;
  boost::uint_fast64_t bytes_written_;
  boost::uint_fast64_t bytes_read_;
  bool was_connected_;
  bool write_in_progress_;
  bool read_in_progress_;
  bool stopped_;
  work_state& work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
}; // class session

typedef boost::shared_ptr<session>     session_ptr;
typedef ma::steady_deadline_timer      deadline_timer;
typedef deadline_timer::duration_type  duration_type;
typedef boost::optional<duration_type> optional_duration;

struct client_config
{
public:
  client_config(std::size_t the_session_count,
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
}; // struct client_config

class client : private boost::noncopyable
{
private:
  typedef client this_type;

public:
  typedef session::protocol protocol;

  client(boost::asio::io_service& io_service, const client_config& config)
    : block_size_(config.block_size)
    , block_pause_(config.block_pause)
    , io_service_(io_service)
    , strand_(io_service)
    , timer_(io_service)
    , sessions_()
    , stopped_(false)
    , timer_in_progess_(false)
    , stats_()
    , work_state_(config.session_count)
  {
    for (std::size_t i = 0; i < config.session_count; ++i)
    {
      sessions_.push_back(boost::make_shared<session>(boost::ref(io_service_),
          config.managed_session_config, boost::ref(work_state_)));
    }
  }

  ~client()
  {
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

  void wait_until_done(const boost::posix_time::time_duration& timeout)
  {
    work_state_.wait_for_all_session_stop(timeout);
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
    BOOST_ASSERT_MSG(!timer_in_progess_, "invalid timer state");

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
}; // class client

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

struct program_config
{
public:
  program_config(const std::string& the_host,
      const std::string& the_port,
      std::size_t the_thread_count,
      const boost::posix_time::time_duration& the_test_duration,
      const client_config& the_program_client_config)
    : host(the_host)
    , port(the_port)
    , thread_count(the_thread_count)
    , test_duration(the_test_duration)
    , program_client_config(the_program_client_config)
  {
  }

  std::string host;
  std::string port;
  std::size_t thread_count;
  boost::posix_time::time_duration test_duration;
  client_config program_client_config;
}; // struct program_config

const char* help_option_name             = "help";
const char* host_option_name             = "host";
const char* port_option_name             = "port";
const char* threads_option_name          = "threads";
const char* sessions_option_name         = "sessions";
const char* block_size_option_name       = "block_size";
const char* block_pause_option_name      = "block_pause";
const char* buffer_option_name           = "buffer";
const char* connect_attempts_option_name = "connect_attempts";
const char* no_delay_option_name         = "no_delay";
const char* time_option_name             = "time";
const std::string default_system_value   = "system default";

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

program_config build_program_config(
    const boost::program_options::variables_map& options_values)
{
  const std::string host = options_values[host_option_name].as<std::string>();
  const std::string port = options_values[port_option_name].as<std::string>();
  const std::size_t thread_count  =
      options_values[threads_option_name].as<std::size_t>();
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
  optional_bool no_delay;
  if (0 != options_values.count(no_delay_option_name))
  {
    no_delay = options_values[no_delay_option_name].as<bool>();
  }
  const long time_seconds =
      options_values[time_option_name].as<long>();

  session_config client_session_config(
      buffer_size, max_connect_attempts, no_delay);

  client_config program_client_config(session_count, block_size,
      to_optional_duration(block_pause_millis), client_session_config);

  return program_config(host, port, thread_count,
      boost::posix_time::seconds(time_seconds), program_client_config);
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

std::string to_bool_string(const optional_bool& no_delay)
{
  if (no_delay)
  {
    return *no_delay ? "on" : "off";
  }
  else
  {
    return "n/a";
  }
}

void print(const program_config& config)
{
  const client_config&  program_client_config =
      config.program_client_config;
  const session_config& managed_session_config =
      program_client_config.managed_session_config;

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
            << program_client_config.session_count
            << std::endl
            << "Block size: "
            << program_client_config.block_size
            << std::endl
            << "Block pause (milliseconds)   : "
            << to_milliseconds_string(program_client_config.block_pause)
            << std::endl
            << "Session's buffer size (bytes): "
            << managed_session_config.buffer_size
            << std::endl
            << "Maximum number of connect attempts per session: "
            << managed_session_config.max_connect_attempts
            << std::endl
            << "TCP_NODELAY   : "
            << to_bool_string(managed_session_config.no_delay)
            << std::endl
            << "Time (seconds): "
            << to_seconds_string(config.test_duration)
            << std::endl;
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

    const program_config config = build_program_config(cmd_options);
    print(config);

    boost::asio::io_service io_service(config.thread_count);
    client::protocol::resolver resolver(io_service);
    client c(io_service, config.program_client_config);
    boost::thread_group work_threads;
    {
      boost::asio::io_service::work io_service_work_guard(io_service);
      for (std::size_t i = 0; i != config.thread_count; ++i)
      {
        work_threads.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service));
      }
      {
#if defined(MA_HAS_BOOST_TIMER)
        boost::timer::auto_cpu_timer cpu_timer("Test duration :" \
            " %ws wall, %us user + %ss system = %ts CPU (%p%)\n");
#endif // defined(MA_HAS_BOOST_TIMER)
        c.async_start(resolver.resolve(
            client::protocol::resolver::query(config.host, config.port)));
        c.wait_until_done(config.test_duration);
      }
      c.async_stop();
    }
    work_threads.join_all();
    return EXIT_SUCCESS;
  }
  catch (const boost::program_options::error& e)
  {
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return EXIT_FAILURE;
}
