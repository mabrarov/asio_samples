//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <string>
#include <cstdlib>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_controller.hpp>
#include <ma/echo/server/session_options.hpp>
#include <ma/echo/server/session_manager_options.hpp>
#include <ma/echo/server/session_manager.hpp>

namespace {

const char* help_option_name = "help"; 
const char* port_option_name = "port";
const char* session_manager_threads_option_name = "session_manager_threads";
const char* session_threads_option_name         = "session_threads";
const char* stop_timeout_option_name            = "stop_timeout";
const char* max_sessions_option_name            = "max_sessions";
const char* recycled_sessions_option_name       = "recycled_sessions";
const char* listen_backlog_option_name          = "listen_backlog";
const char* buffer_size_option_name             = "buffer";
const char* inactivity_timeout_option_name      = "inactivity_timeout";
const char* socket_recv_buffer_size_option_name = "sock_recv_buffer";
const char* socket_send_buffer_size_option_name = "sock_send_buffer";
const char* socket_no_delay_option_name         = "sock_no_delay";
const std::string default_system_value          = "system default";
 
template <typename Value>
void validate_option(const std::string& option_name, const Value& option_value,
    const Value& min, const Value& max = (std::numeric_limits<Value>::max)())
{
  if ((option_value < min) || (option_value > max))
  {
    using boost::program_options::validation_error;
    boost::throw_exception(validation_error(
        validation_error::invalid_option_value, std::string(), option_name));
  }
}

template <typename Value>
void print_optional(std::ostream& stream, const boost::optional<Value>& value,
    const std::string& default_text)
{
  if (value) 
  {
    stream << *value;
  }
  else 
  {
    stream << default_text;
  }
}

template <>
void print_optional<bool>(std::ostream& stream, 
    const boost::optional<bool>& value, const std::string& default_text)
{
  if (value) 
  {
    if (*value)
    {
      stream << "on";
    }
    else 
    {
      stream << "off";
    }    
  }
  else 
  {
    stream << default_text;
  }
}

struct  session_manager_data;
typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
typedef boost::function<void (void)> exception_handler;  

class execution_options
{
public:    
  typedef boost::posix_time::time_duration time_duration_type;
    
  execution_options(std::size_t session_manager_thread_count,
      std::size_t session_thread_count, const time_duration_type& stop_timeout)
    : session_manager_thread_count_(session_manager_thread_count)
    , session_thread_count_(session_thread_count)
    , stop_timeout_(stop_timeout)
  {
    BOOST_ASSERT_MSG(session_manager_thread_count > 0, 
        "session_manager_thread_count must be > 0");
    BOOST_ASSERT_MSG(session_thread_count > 0, 
        "session_thread_count must be > 0");
  }
    
  std::size_t session_manager_thread_count() const
  {
    return session_manager_thread_count_;
  }

  std::size_t session_thread_count() const
  {
    return session_thread_count_;
  }

  time_duration_type stop_timeout() const
  {
    return stop_timeout_;
  }

private:
  std::size_t session_manager_thread_count_;
  std::size_t session_thread_count_;
  time_duration_type stop_timeout_;
}; // class execution_options

struct session_manager_data : private boost::noncopyable
{
  typedef boost::mutex mutex_type;

  enum state_type
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  }; // enum state_type

  volatile bool stopped_by_user;
  volatile state_type state;
  mutex_type access_mutex;  
  ma::echo::server::session_manager_ptr session_manager;    
  boost::condition_variable state_changed;  
  ma::in_place_handler_allocator<128> start_wait_allocator;
  ma::in_place_handler_allocator<128> stop_allocator;

  session_manager_data(boost::asio::io_service& io_service,
      boost::asio::io_service& session_io_service,
      const ma::echo::server::session_manager_options& options)
    : stopped_by_user(false)
    , state(ready_to_start)
    , session_manager(boost::make_shared<ma::echo::server::session_manager>(
          boost::ref(io_service), boost::ref(session_io_service), options))
  {
  }

  ~session_manager_data()
  {
  }
}; // session_manager_data

std::size_t calc_session_manager_thread_count(
    std::size_t hardware_concurrency);

std::size_t calc_session_thread_count(std::size_t hardware_concurrency);

void fill_options_description(boost::program_options::options_description&, 
    std::size_t hardware_concurrency);

void start_session_manager(const session_manager_data_ptr&);
void wait_session_manager(const session_manager_data_ptr&);
void stop_session_manager(const session_manager_data_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_manager_data_ptr&);
void handle_program_exit(const session_manager_data_ptr&);
void handle_session_manager_start(const session_manager_data_ptr&, 
    const boost::system::error_code&);
void handle_session_manager_wait(const session_manager_data_ptr&, 
    const boost::system::error_code&);
void handle_session_manager_stop(const session_manager_data_ptr&, 
    const boost::system::error_code&);

execution_options read_execution_options(
    const boost::program_options::variables_map& options_values);

ma::echo::server::session_options read_session_options(
    const boost::program_options::variables_map& options_values);

ma::echo::server::session_manager_options read_session_manager_options(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_options& session_options);

void print_options(std::ostream& stream, std::size_t cpu_count,
    const execution_options& the_execution_options,
    const ma::echo::server::session_manager_options& session_manager_options);

void handle_session_manager_start(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
{   
  boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(
      the_session_manager_data->access_mutex);

  if (session_manager_data::start_in_progress 
      == the_session_manager_data->state)
  {     
    if (error)
    {       
      the_session_manager_data->state = session_manager_data::stopped;

      std::cout << "Server can't start due to error: " 
          << error.message() << '\n';

      wrapper_lock.unlock();
      the_session_manager_data->state_changed.notify_one();
      return;
    }

    the_session_manager_data->state = session_manager_data::started;
    wait_session_manager(the_session_manager_data);
    std::cout << "Server has started.\n";
  }
}

void handle_session_manager_wait(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
{
  boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(
      the_session_manager_data->access_mutex);

  if (session_manager_data::started == the_session_manager_data->state)
  {
    stop_session_manager(the_session_manager_data);

    std::cout << "Server can't continue work due to error: " 
        << error.message() << '\n' 
        << "Server is stopping...\n";

    wrapper_lock.unlock();
    the_session_manager_data->state_changed.notify_one();    
  }
}

void handle_session_manager_stop(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code&)
{
  boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(
      the_session_manager_data->access_mutex);

  if (session_manager_data::stop_in_progress 
      == the_session_manager_data->state)
  {      
    the_session_manager_data->state = session_manager_data::stopped;

    std::cout << "Server has stopped.\n";    

    wrapper_lock.unlock();
    the_session_manager_data->state_changed.notify_one();
  }
}  
  
execution_options read_execution_options(
    const boost::program_options::variables_map& options_values)
{
  std::size_t session_manager_thread_count = 
      options_values[session_manager_threads_option_name].as<std::size_t>();

  validate_option<std::size_t>(session_manager_threads_option_name, 
      session_manager_thread_count, 1);

  std::size_t session_thread_count = 
      options_values[session_threads_option_name].as<std::size_t>();

  validate_option<std::size_t>(session_threads_option_name, 
      session_thread_count, 1);

  long stop_timeout_sec = options_values[stop_timeout_option_name].as<long>();

  validate_option<long>(stop_timeout_option_name, stop_timeout_sec, 0);
    
  return execution_options(session_manager_thread_count, session_thread_count, 
      boost::posix_time::seconds(stop_timeout_sec));
} 

boost::optional<int> read_socket_buffer_size(
    const boost::program_options::variables_map& options_values,
    const std::string& option_name)
{     
  if (!options_values.count(option_name))
  {
    return boost::optional<int>();
  }
  int buffer_size = options_values[option_name].as<int>();
  validate_option<int>(option_name, buffer_size, 0);
  return buffer_size;
}

ma::echo::server::session_options read_session_options(
    const boost::program_options::variables_map& options_values)
{ 
  using ma::echo::server::session_options;

  boost::optional<bool> no_delay;
  if (options_values.count(socket_no_delay_option_name))
  {
    no_delay = !options_values[socket_no_delay_option_name].as<bool>();
  }

  std::size_t buffer_size = 
      options_values[buffer_size_option_name].as<std::size_t>();
  validate_option<std::size_t>(buffer_size_option_name, buffer_size, 1);
    
  session_options::optional_time_duration inactivity_timeout;
  if (options_values.count(inactivity_timeout_option_name))
  {
    long timeout_sec = 
        options_values[inactivity_timeout_option_name].as<long>();
    validate_option<long>(inactivity_timeout_option_name, timeout_sec, 0);
    inactivity_timeout = boost::posix_time::seconds(timeout_sec);
  }

  boost::optional<int> socket_recv_buffer_size = read_socket_buffer_size(
      options_values, socket_recv_buffer_size_option_name);

  boost::optional<int> socket_send_buffer_size = read_socket_buffer_size(
      options_values, socket_send_buffer_size_option_name);
    
  return session_options(buffer_size, socket_recv_buffer_size, 
      socket_send_buffer_size, no_delay, inactivity_timeout);
}

ma::echo::server::session_manager_options read_session_manager_options(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_options& session_options)
{    
  unsigned short port = options_values[port_option_name].as<unsigned short>();
    
  std::size_t max_sessions = 
      options_values[max_sessions_option_name].as<std::size_t>();

  validate_option<std::size_t>(max_sessions_option_name, max_sessions, 1);

  std::size_t recycled_sessions = 
      options_values[recycled_sessions_option_name].as<std::size_t>();

  int listen_backlog = options_values[listen_backlog_option_name].as<int>();    
    
  using boost::asio::ip::tcp;

  return ma::echo::server::session_manager_options(
      tcp::endpoint(tcp::v4(), port), max_sessions, recycled_sessions, 
      listen_backlog, session_options);
}

void print_options(std::ostream& stream, std::size_t cpu_count,
    const execution_options& the_execution_options,
    const ma::echo::server::session_manager_options& session_manager_options)
{
  using ma::echo::server::session_options;

  const session_options the_session_options = 
      session_manager_options.managed_session_options();

  stream << "Number of found CPU(s)                : " 
         << cpu_count 
         << std::endl
         << "Number of session manager's threads   : " 
         << the_execution_options.session_manager_thread_count() 
         << std::endl
         << "Number of sessions' threads           : " 
         << the_execution_options.session_thread_count() 
         << std::endl
         << "Total number of work threads          : " 
         << the_execution_options.session_thread_count() 
                + the_execution_options.session_manager_thread_count() 
         << std::endl
         << "Server listen port                    : " 
         << session_manager_options.accepting_endpoint().port()
         << std::endl
         << "Server stop timeout (seconds)         : " 
         << the_execution_options.stop_timeout().total_seconds() 
         << std::endl
         << "Maximum number of active sessions     : " 
         << session_manager_options.max_session_count()
         << std::endl
         << "Maximum number of recycled sessions   : "
         << session_manager_options.recycled_session_count()
         << std::endl
         << "TCP listen backlog size               : "
         << session_manager_options.listen_backlog()
         << std::endl
         << "Size of session's buffer (bytes)      : " 
         << the_session_options.buffer_size()
         << std::endl;
    
  boost::optional<long> session_inactivity_timeout_sec;
  if (session_options::optional_time_duration timeout = 
      the_session_options.inactivity_timeout())
  {
    session_inactivity_timeout_sec = timeout->total_seconds();
  }    

  stream << "Session's inactivity timeout (seconds): ";
  print_optional(stream, session_inactivity_timeout_sec, "none");
  stream << std::endl;

  stream << "Size of session's socket receive buffer (bytes): ";
  print_optional(stream, the_session_options.socket_recv_buffer_size(), 
      default_system_value);
  stream << std::endl;

  stream << "Size of session's socket send buffer (bytes)   : ";
  print_optional(stream, the_session_options.socket_send_buffer_size(), 
      default_system_value);
  stream << std::endl;

  stream << "Session's socket Nagle algorithm is   : ";
  print_optional(stream, the_session_options.no_delay(), default_system_value);
  stream << std::endl;      
}

std::size_t calc_session_manager_thread_count(std::size_t hardware_concurrency)
{
  if (hardware_concurrency < 2)
  {
    return 1;
  }
  return 2;
}

std::size_t calc_session_thread_count(std::size_t hardware_concurrency)
{
  if (hardware_concurrency)
  {
    if ((std::numeric_limits<std::size_t>::max)() == hardware_concurrency)
    {
      return hardware_concurrency;
    }
    return hardware_concurrency + 1;
  }
  return 2;
}

void fill_options_description(
    boost::program_options::options_description& options_description, 
    std::size_t hardware_concurrency)
{
  std::size_t session_manager_thread_count = 
      calc_session_manager_thread_count(hardware_concurrency);

  std::size_t session_thread_count = 
      calc_session_thread_count(hardware_concurrency);

  options_description.add_options()
    (
      help_option_name, 
      "produce help message"
    )
    (
      port_option_name, 
      boost::program_options::value<unsigned short>(), 
      "set the TCP port number for incoming connections' listening"
    )
    (
      session_manager_threads_option_name, 
      boost::program_options::value<std::size_t>()->default_value(
          session_manager_thread_count), 
      "set the number of session manager's threads"
    )    
    (
      session_threads_option_name, 
      boost::program_options::value<std::size_t>()->default_value(
          session_thread_count), 
      "set the number of sessions' threads"
    )    
    (
      stop_timeout_option_name, 
      boost::program_options::value<long>()->default_value(60), 
      "set the server stop timeout at one's expiration server work" \
          " will be terminated (seconds)"
    )
    (
      max_sessions_option_name, 
      boost::program_options::value<std::size_t>()->default_value(10000), 
      "set the maximum number of simultaneously active sessions"
    )
    (
      recycled_sessions_option_name, 
      boost::program_options::value<std::size_t>()->default_value(100), 
      "set the maximum number of pooled inactive sessions"
    )
    (
      listen_backlog_option_name, 
      boost::program_options::value<int>()->default_value(6), 
      "set the size of TCP listen backlog"
    )
    (
      buffer_size_option_name, 
      boost::program_options::value<std::size_t>()->default_value(1024),
      "set the session's buffer size (bytes)"
    )
    (
      inactivity_timeout_option_name, 
      boost::program_options::value<long>(), 
      "set the timeout at one's expiration session will be considered" \
          " as inactive and will be closed (seconds)"
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
      socket_no_delay_option_name, 
      boost::program_options::value<bool>(),
      "set TCP_NODELAY option of session's socket"
    );  
}

void start_session_manager(
    const session_manager_data_ptr& the_session_manager_data)
{
  the_session_manager_data->session_manager->async_start(
      ma::make_custom_alloc_handler(
          the_session_manager_data->start_wait_allocator, boost::bind(
              handle_session_manager_start, the_session_manager_data, _1)));

  the_session_manager_data->state = session_manager_data::start_in_progress;
}

void wait_session_manager(
    const session_manager_data_ptr& the_session_manager_data)
{
  the_session_manager_data->session_manager->async_wait(
      ma::make_custom_alloc_handler(
          the_session_manager_data->start_wait_allocator, boost::bind(
              handle_session_manager_wait, the_session_manager_data, _1)));
}
  
void stop_session_manager(
    const session_manager_data_ptr& the_session_manager_data)
{
  the_session_manager_data->session_manager->async_stop(
      ma::make_custom_alloc_handler(
          the_session_manager_data->stop_allocator, boost::bind(
              handle_session_manager_stop, the_session_manager_data, _1)));

  the_session_manager_data->state = session_manager_data::stop_in_progress;
}

void run_io_service(boost::asio::io_service& io_service, 
    exception_handler the_exception_handler)
{
  try 
  {
    io_service.run();
  }
  catch (...)
  {
    the_exception_handler();
  }
}

void handle_work_exception(
    const session_manager_data_ptr& the_session_manager_data)
{
  boost::unique_lock<session_manager_data::mutex_type> 
      wrapper_lock(the_session_manager_data->access_mutex);

  if (session_manager_data::stopped != the_session_manager_data->state)
  {
    the_session_manager_data->state = session_manager_data::stopped;  

    std::cout << "Terminating server work due to unexpected exception...\n";

    wrapper_lock.unlock();
    the_session_manager_data->state_changed.notify_one();      
  }  
}

void handle_program_exit(
    const session_manager_data_ptr& the_session_manager_data)
{
  boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(
      the_session_manager_data->access_mutex);

  std::cout << "Program exit request detected.\n";

  if (session_manager_data::stopped == the_session_manager_data->state)
  {    
    std::cout << "Server has already stopped.\n";
    return;
  }

  if (session_manager_data::stop_in_progress 
      == the_session_manager_data->state)
  {    
    the_session_manager_data->state = session_manager_data::stopped;

    std::cout << "Server is already stopping. Terminating server work...\n";

    wrapper_lock.unlock();
    the_session_manager_data->state_changed.notify_one();       
    return;
  } 

  // Begin server stop
  stop_session_manager(the_session_manager_data);
  the_session_manager_data->stopped_by_user = true;

  std::cout << "Server is stopping." \
      " Press Ctrl+C (Ctrl+Break) to terminate server work...\n";

  wrapper_lock.unlock();    
  the_session_manager_data->state_changed.notify_one();  
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
    std::size_t cpu_count = boost::thread::hardware_concurrency();

    // Initialize options description... 
    boost::program_options::options_description options_description(
        "Allowed options");
    fill_options_description(options_description, cpu_count);
    // ... and read options values
    boost::program_options::variables_map options_values;  
    boost::program_options::store(boost::program_options::parse_command_line(
        argc, argv, options_description), options_values);
    boost::program_options::notify(options_values);
    
    // Check start mode
    if (options_values.count(help_option_name))
    {
      std::cout << options_description;
      return EXIT_SUCCESS;
    }
    if (!options_values.count(port_option_name))
    {          
      std::cout << "Port must be specified" << std::endl 
          << options_description;
      return EXIT_FAILURE;
    }        

    // Read configuration
    execution_options the_execution_options =
        read_execution_options(options_values);    
    ma::echo::server::session_options session_options = 
        read_session_options(options_values);
    ma::echo::server::session_manager_options session_manager_options = 
        read_session_manager_options(options_values, session_options);

    print_options(std::cout, cpu_count, the_execution_options, 
        session_manager_options);
                  
    // Before session_manager_io_service
    boost::asio::io_service session_io_service(
        the_execution_options.session_thread_count());
    // ... for the right destruction order
    boost::asio::io_service session_manager_io_service(
        the_execution_options.session_manager_thread_count());

    session_manager_data_ptr the_session_manager_data(
        boost::make_shared<session_manager_data>(
            boost::ref(session_manager_io_service), 
            boost::ref(session_io_service), session_manager_options));
      
    std::cout << "Server is starting...\n";
      
    // Start the server      
    boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
        the_session_manager_data->access_mutex);    
    start_session_manager(the_session_manager_data);
    session_manager_lock.unlock();
      
    // Setup console controller
    ma::console_controller console_controller(
        boost::bind(handle_program_exit, the_session_manager_data));
    std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler work_exception_handler(
        boost::bind(handle_work_exception, the_session_manager_data));

    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for session_manager's io_service to prevent threads' stop
    boost::asio::io_service::work session_manager_work(
        session_manager_io_service);

    // Create work threads for session operations
    boost::thread_group work_threads;    
    for (std::size_t i = 0; 
        i != the_execution_options.session_thread_count(); ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
          boost::ref(session_io_service), work_exception_handler));
    }
    // Create work threads for session manager operations
    for (std::size_t i = 0; 
        i != the_execution_options.session_manager_thread_count(); ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
          boost::ref(session_manager_io_service), work_exception_handler));
    }

    int exit_code = EXIT_SUCCESS;

    // Wait until server begins stopping or stops
    session_manager_lock.lock();
    while (
        (session_manager_data::stop_in_progress 
            != the_session_manager_data->state)
        && (session_manager_data::stopped 
            != the_session_manager_data->state))
    {
      the_session_manager_data->state_changed.wait(session_manager_lock);
    }        
    if (session_manager_data::stopped != the_session_manager_data->state)
    {
      // Wait until server stops with timeout
      if (!the_session_manager_data->state_changed.timed_wait(
          session_manager_lock, the_execution_options.stop_timeout()))
      {
        std::cout << "Server stop timeout expiration." \
            " Terminating server work...\n";
        exit_code = EXIT_FAILURE;
      }
      // Mark server as stopped
      the_session_manager_data->state = session_manager_data::stopped;
    }
    // Check the reason of server stop and signal it by exit code
    if ((EXIT_SUCCESS == exit_code)
        && !the_session_manager_data->stopped_by_user)
    {      
      exit_code = EXIT_FAILURE;
    }
    session_manager_lock.unlock();
     
    // Shutdown execution queues...
    session_manager_io_service.stop();
    session_io_service.stop();

    std::cout << "Server work was terminated." \
        " Waiting until all of the work threads will stop...\n";

    // ...and wait until all work done.
    work_threads.join_all();
    std::cout << "Work threads have stopped. Process will close.\n";

    return exit_code;
  }  
  catch (const boost::program_options::error& e)
  {    
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {    
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}
