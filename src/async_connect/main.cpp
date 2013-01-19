//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <exception>
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
  {
  }

  void add(const limited_counter& connect_count)
  {
    total_sessions_connected_ += connect_count;
  }

  void print()
  {
    std::cout << "Total sessions connected: "
              << to_string(total_sessions_connected_)
              << std::endl;
  }

private:
  limited_counter total_sessions_connected_;
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
    , connect_count_()
    , connected_(false)
    , started_(false)
    , stopped_(false)
    , timer_in_progess_(false)
    , work_state_(work_state)
  {
  }

  ~session()
  {
    BOOST_ASSERT_MSG(!connected_, "Invalid connect state");
    BOOST_ASSERT_MSG(!timer_in_progess_, "Timer wait is still in progress");
    BOOST_ASSERT_MSG(!started_ || (started_ && stopped_),
        "Session was not stopped");
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

  limited_counter connect_count() const
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

    started_ = true;
    start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
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
    // Collect statistics at first step
    if (!error)
    {
      ++connect_count_;
    }

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

    connected_ = true;

    if (apply_socket_options())
    {
      stop();
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
    BOOST_ASSERT_MSG(!timer_in_progess_, "Invalid timer state");

    timer_.expires_from_now(*connect_pause_);
    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_,
            boost::bind(&this_type::handle_timer, this, _1,
                initial_endpoint_iterator))));
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
      stop();
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

  void stop()
  {
    close_socket();
    connected_ = false;
    cancel_timer();
    stopped_ = true;
    work_state_.dec_outstanding();
  }

  const optional_duration connect_pause_;
  const optional_bool     no_delay_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  deadline_timer   timer_;
  limited_counter  connect_count_;
  bool connected_;
  bool started_;
  bool stopped_;
  bool timer_in_progess_;
  work_state&      work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> connect_allocator_;
  ma::in_place_handler_allocator<256> timer_allocator_;
}; // class session

typedef boost::shared_ptr<session> session_ptr;

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
    , stopped_(false)
    , timer_in_progess_(false)
    , work_state_(config.session_count)
  {
    typedef io_service_vector::const_iterator iterator;

    sessions_.reserve(config.session_count);
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
    started_sessions_end_ = sessions_.begin();
  }

  ~session_manager()
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "Invalid timer state");

    std::for_each(session_vector::const_iterator(sessions_.begin()),
        started_sessions_end_,
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

  static session_vector::const_iterator start_sessions(
      const protocol::resolver::iterator& endpoint_iterator,
      const session_vector::const_iterator& begin,
      const session_vector::const_iterator& end,
      std::size_t max_count)
  {
    session_vector::const_iterator i = begin;
    std::size_t count = 0;
    for (; (end != i) && (count != max_count); ++i, ++count)
    {
      (*i)->async_start(endpoint_iterator);
    }
    return i;
  }

  void do_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    started_sessions_end_ = start_sessions(endpoint_iterator,
        started_sessions_end_, sessions_.end(), block_size_);
    if (sessions_.end() != started_sessions_end_)
    {
      schedule_session_start(endpoint_iterator);
    }
  }

  void schedule_session_start(
      const protocol::resolver::iterator& endpoint_iterator)
  {
    if (block_pause_)
    {
      BOOST_ASSERT_MSG(!timer_in_progess_, "Invalid timer state");

      timer_.expires_from_now(*block_pause_);
      timer_.async_wait(MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(timer_allocator_,
              boost::bind(&this_type::handle_scheduled_session_start, this,
                  _1, endpoint_iterator))));
      timer_in_progess_ = true;
    }
    else
    {
      strand_.post(ma::make_custom_alloc_handler(timer_allocator_,
          boost::bind(&this_type::handle_scheduled_session_start, this,
              boost::system::error_code(), endpoint_iterator)));
    }
  }

  void handle_scheduled_session_start(const boost::system::error_code& error,
      const protocol::resolver::iterator& endpoint_iterator)
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

    started_sessions_end_ = start_sessions(endpoint_iterator,
        started_sessions_end_, sessions_.end(), block_size_);
    if (sessions_.end() != started_sessions_end_)
    {
      schedule_session_start(endpoint_iterator);
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
    std::for_each(session_vector::const_iterator(sessions_.begin()),
        started_sessions_end_, boost::bind(&session::async_stop, _1));
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
  session_vector::const_iterator started_sessions_end_;
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

const char* help_option_name           = "help";
const char* host_option_name           = "host";
const char* port_option_name           = "port";
const char* demux_option_name          = "demux_per_work_thread";
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

#if defined(WIN32)
boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[])
#else
boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[])
#endif
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

client_config build_client_config(
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
            << "Block pause (milliseconds)  : "
            << to_milliseconds_string(
                  client_session_manager_config.block_pause)
            << std::endl
            << "Demultiplexer-per-work-thread mode: "
            << (to_string)(config.ios_per_work_thread)
            << std::endl
            << "Connect pause (milliseconds): "
            << to_milliseconds_string(managed_session_config.connect_pause)
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

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
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
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}
