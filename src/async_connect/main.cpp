//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
#include <ma/async_connect.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>

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
  {
  }

  void add(std::size_t connect_count)
  {
    total_sessions_connected_ += connect_count;
  }

  void print()
  {
    std::cout << "Total sessions connected: "
              << total_sessions_connected_ << std::endl;
  }

private:
  boost::uint_fast64_t total_sessions_connected_;
}; // class stats

typedef ma::steady_deadline_timer      deadline_timer;
typedef deadline_timer::duration_type  duration_type;
typedef boost::optional<duration_type> optional_duration;
typedef boost::optional<bool>          optional_bool;

struct session_config
{
public:
  session_config(const optional_duration& the_connect_pause,
      const optional_bool& the_no_delay)
    : connect_pause(the_connect_pause)
    , no_delay(the_no_delay)
  {
  }

  optional_duration connect_pause;
  optional_bool     no_delay;
}; // struct session_config

class session : private boost::noncopyable
{
  typedef session this_type;

public:
  typedef boost::asio::ip::tcp protocol;

  session(boost::asio::io_service& io_service, const session_config& config,
      work_state& work_state)
    : connect_pause_(config.connect_pause)
    , no_delay_(config.no_delay)
    , strand_(io_service)
    , socket_(io_service)
    , timer_(io_service)
    , connect_count_(0)
    , stopped_(false)
    , timer_in_progess_(false)
    , work_state_(work_state)
  {
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(connect_allocator_,
        boost::bind(&this_type::do_start, this, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  std::size_t connect_count() const
  {
    return connect_count_;
  }

private:
  void do_start(const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
  }

  void start_connect(
      const protocol::resolver::iterator& initial_endpoint_iterator,
      const protocol::resolver::iterator& current_endpoint_iterator)
  {
    protocol::endpoint endpoint = *current_endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(connect_allocator_,
            boost::bind(&this_type::handle_connect, this, _1,
                initial_endpoint_iterator, current_endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& error,
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
        start_connect(initial_endpoint_iterator, current_endpoint_iterator);
      }
      else
      {
        start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
      }
      return;
    }

    ++connect_count_;
    if (boost::system::error_code error = apply_socket_options())
    {
      do_stop();
      return;
    }

    if (connect_pause_)
    {
      start_deferred_continue_work(initial_endpoint_iterator);
    }
    else
    {
      continue_work(initial_endpoint_iterator);
    }
  }

  void start_deferred_continue_work(
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "invalid timer state");

    timer_.expires_from_now(*connect_pause_);
    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_,
            boost::bind(&this_type::handle_timer, this,
                _1, initial_endpoint_iterator))));
    timer_in_progess_ = true;
  }

  void handle_timer(const boost::system::error_code& error,
      const protocol::resolver::iterator& initial_endpoint_iterator)
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

    continue_work(initial_endpoint_iterator);
  }

  void continue_work(
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    shutdown_socket();
    close_socket();
    start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
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

    close_socket();
    cancel_timer();
    stopped_ = true;
    work_state_.notify_session_stop();
  }

  const optional_duration connect_pause_;
  const optional_bool     no_delay_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  deadline_timer   timer_;
  std::size_t      connect_count_;
  bool             stopped_;
  bool             timer_in_progess_;
  work_state&      work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> connect_allocator_;
  ma::in_place_handler_allocator<256> timer_allocator_;
}; // class session

typedef boost::shared_ptr<session> session_ptr;

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
    stats_.add(session->connect_count());
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

const char* help_option_name           = "help";
const char* host_option_name           = "host";
const char* port_option_name           = "port";
const char* threads_option_name        = "threads";
const char* sessions_option_name       = "sessions";
const char* block_size_option_name     = "block_size";
const char* block_pause_option_name    = "block_pause";
const char* connect_pause_option_name  = "connect_pause";
const char* no_delay_option_name       = "no_delay";
const char* time_option_name           = "time";
const std::string default_system_value = "system default";

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
      connect_pause_option_name,
      boost::program_options::value<long>()->default_value(1000),
      "set the pause before next connect operation" \
          " for each session (milliseconds)"
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
  const long connect_pause_millis =
      options_values[connect_pause_option_name].as<long>();
  optional_bool no_delay;
  if (0 != options_values.count(no_delay_option_name))
  {
    no_delay = options_values[no_delay_option_name].as<bool>();
  }
  const long time_seconds =
      options_values[time_option_name].as<long>();

  session_config client_session_config(
      to_optional_duration(connect_pause_millis), no_delay);

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

std::string to_milliseconds_string(const duration_type& duration)
{
  return to_milliseconds_string(
      deadline_timer::traits_type::to_posix_duration(duration));
}

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

std::string to_string(const optional_bool& no_delay)
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
            << "Block pause (milliseconds)  : " 
            << to_milliseconds_string(program_client_config.block_pause)
            << std::endl
            << "Connect pause (milliseconds): " 
            << to_milliseconds_string(managed_session_config.connect_pause)
            << std::endl
            << "TCP_NODELAY                 : "
            << to_string(managed_session_config.no_delay)
            << std::endl
            << "Time (seconds)              : " 
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
    c.async_start(resolver.resolve(
        client::protocol::resolver::query(config.host, config.port)));

    boost::thread_group work_threads;
    for (std::size_t i = 0; i != config.thread_count; ++i)
    {
      work_threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service));
    }

    c.wait_until_done(config.test_duration);
    c.async_stop();

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
