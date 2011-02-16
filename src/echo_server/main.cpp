//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/console_controller.hpp>

const char* help_param = "help"; 
const char* port_param = "port";
const char* session_threads_param   = "session_threads";
const char* stop_timeout_param      = "stop_timeout";
const char* max_sessions_param      = "max_sessions";
const char* recycled_sessions_param = "recycled_sessions";
const char* listen_backlog_param    = "listen_backlog";
const char* buffer_size_param       = "buffer";
const char* socket_recv_buffer_size_param = "socket_recv_buffer";
const char* socket_send_buffer_size_param = "socket_send_buffer";
const char* socket_no_delay_param   = "socket_no_delay";
const char* os_default_text = "OS default";

namespace server = ma::echo::server;

struct  session_manager_wrapper;
typedef boost::shared_ptr<session_manager_wrapper> session_manager_wrapper_ptr;
typedef boost::function<void (void)> exception_handler;

struct execution_config
{
  std::size_t session_manager_thread_count;
  std::size_t session_thread_count;
  boost::posix_time::time_duration stop_timeout;

  execution_config(std::size_t the_session_manager_thread_count,
    std::size_t the_session_thread_count, 
    const boost::posix_time::time_duration& the_stop_timeout)
    : session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)
    , stop_timeout(the_stop_timeout)
  {
    if (the_session_manager_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument(
        "the_session_manager_thread_count must be >= 1"));
    }
    if (the_session_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument(
        "the_session_thread_count must be >= 1"));
    }
  }
}; // struct execution_config

struct session_manager_wrapper : private boost::noncopyable
{  
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
  boost::mutex access_mutex;  
  server::session_manager_ptr the_session_manager;    
  boost::condition_variable state_changed;  
  ma::in_place_handler_allocator<128> start_wait_allocator;
  ma::in_place_handler_allocator<128> stop_allocator;

  session_manager_wrapper(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    const server::session_manager_config& config)
    : stopped_by_user(false)
    , state(ready_to_start)
    , the_session_manager(boost::make_shared<server::session_manager>(
        boost::ref(io_service), boost::ref(session_io_service), config))        
  {
  }

  ~session_manager_wrapper()
  {
  }
}; // session_manager_wrapper

void fill_options_description(boost::program_options::options_description&,
  std::size_t cpu_count);

void start_session_manager(const session_manager_wrapper_ptr&);

void wait_session_manager(const session_manager_wrapper_ptr&);

void stop_session_manager(const session_manager_wrapper_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_manager_wrapper_ptr&);

void handle_program_exit(const session_manager_wrapper_ptr&);

void handle_session_manager_start(const session_manager_wrapper_ptr&,
  const boost::system::error_code&);

void handle_session_manager_wait(const session_manager_wrapper_ptr&,
  const boost::system::error_code&);

void handle_session_manager_stop(const session_manager_wrapper_ptr&,
  const boost::system::error_code&);

execution_config create_execution_config(std::size_t cpu_count,
  const boost::program_options::variables_map& options_values);

server::session_config create_session_config(
  const boost::program_options::variables_map& options_values);

server::session_manager_config create_session_manager_config(
  const boost::program_options::variables_map& options_values,
  const server::session_config& session_config);

void print_config(std::ostream& stream, std::size_t cpu_count,
  const execution_config& the_execution_config,
  const server::session_manager_config& session_manager_config);

template <typename Value>
void print_param(std::ostream& stream, 
  const boost::optional<Value>& value, 
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
} // print_param

template <>
void print_param<bool>(std::ostream& stream, 
  const boost::optional<bool>& value, 
  const std::string& default_text)
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
} // print_param

int _tmain(int argc, _TCHAR* argv[])
{
  try 
  {    
    std::size_t cpu_count = boost::thread::hardware_concurrency();

    // Initialize options description and read options values
    boost::program_options::options_description options_description("Allowed options");
    fill_options_description(options_description, cpu_count);
    boost::program_options::variables_map options_values;  
    boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), options_values);
    boost::program_options::notify(options_values);
    
    // Check start mode
    if (options_values.count(help_param))
    {
      std::cout << options_description;
      return EXIT_SUCCESS;
    }
    if (!options_values.count(port_param))
    {          
      std::cout << "Port must be specified" << std::endl << options_description;
      return EXIT_FAILURE;
    }        

    // Create configuration
    execution_config the_execution_config = create_execution_config(cpu_count, options_values);    
    server::session_config session_config = create_session_config(options_values);
    server::session_manager_config session_manager_config = 
      create_session_manager_config(options_values, session_config);

    print_config(std::cout, cpu_count, the_execution_config, session_manager_config);
                  
    // Before session_manager_io_service
    boost::asio::io_service session_io_service(the_execution_config.session_thread_count);      
    // ... for the right destruction order
    boost::asio::io_service session_manager_io_service(the_execution_config.session_manager_thread_count);

    session_manager_wrapper_ptr wrapped_session_manager(boost::make_shared<session_manager_wrapper>(
      boost::ref(session_manager_io_service), boost::ref(session_io_service), session_manager_config));
      
    std::cout << "Server is starting...\n";
      
    // Start the server      
    boost::unique_lock<boost::mutex> session_manager_lock(wrapped_session_manager->access_mutex);    
    start_session_manager(wrapped_session_manager);
    session_manager_lock.unlock();
      
    // Setup console controller
    ma::console_controller console_controller(boost::bind(handle_program_exit, wrapped_session_manager));
    std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler work_exception_handler(boost::bind(handle_work_exception, wrapped_session_manager));

    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for session_manager's io_service to prevent threads' stop
    boost::asio::io_service::work session_manager_work(session_manager_io_service);    

    // Create work threads for session operations
    boost::thread_group work_threads;    
    for (std::size_t i = 0; i != the_execution_config.session_thread_count; ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_io_service), work_exception_handler));
    }
    // Create work threads for session manager operations
    for (std::size_t i = 0; i != the_execution_config.session_manager_thread_count; ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_manager_io_service), work_exception_handler));      
    }

    int exit_code = EXIT_SUCCESS;

    // Wait until server begins stopping or stops
    session_manager_lock.lock();
    while (session_manager_wrapper::stop_in_progress != wrapped_session_manager->state 
      && session_manager_wrapper::stopped != wrapped_session_manager->state)
    {
      wrapped_session_manager->state_changed.wait(session_manager_lock);
    }        
    if (session_manager_wrapper::stopped != wrapped_session_manager->state)
    {
      // Wait until server stops with timeout
      if (!wrapped_session_manager->state_changed.timed_wait(
        session_manager_lock, the_execution_config.stop_timeout))      
      {
        std::cout << "Server stop timeout expiration. Terminating server work...\n";
        exit_code = EXIT_FAILURE;
      }
      // Mark server as stopped
      wrapped_session_manager->state = session_manager_wrapper::stopped;
    }
    // Check the reason of server stop and signal it by exit code
    if (EXIT_SUCCESS == exit_code && !wrapped_session_manager->stopped_by_user)
    {      
      exit_code = EXIT_FAILURE;
    }
    session_manager_lock.unlock();
     
    // Shutdown execution queues...
    session_manager_io_service.stop();
    session_io_service.stop();
    std::cout << "Server work was terminated. Waiting until all of the work threads will stop...\n";
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
} // _tmain

execution_config create_execution_config(std::size_t cpu_count,
  const boost::program_options::variables_map& options_values)
{
  if (!cpu_count)
  {
    cpu_count = 1;
  }
  return execution_config(1, options_values[session_threads_param].as<std::size_t>(),
    boost::posix_time::seconds(options_values[stop_timeout_param].as<long>()));
} // create_execution_config

server::session_config create_session_config(
  const boost::program_options::variables_map& options_values)
{  
  boost::optional<bool> no_delay;
  if (options_values.count(socket_no_delay_param))
  {
    no_delay = !options_values[socket_no_delay_param].as<bool>();
  }
  return server::session_config(options_values[buffer_size_param].as<std::size_t>(),
    options_values.count(socket_recv_buffer_size_param) ? 
    options_values[socket_recv_buffer_size_param].as<int>() : boost::optional<int>(),
    options_values.count(socket_send_buffer_size_param) ? 
    options_values[socket_send_buffer_size_param].as<int>() : boost::optional<int>(),
    no_delay);
} // create_session_config

server::session_manager_config create_session_manager_config(
  const boost::program_options::variables_map& options_values,
  const server::session_config& session_config)
{
  using boost::asio::ip::tcp;
  unsigned short port = options_values[port_param].as<unsigned short>();
  return server::session_manager_config(tcp::endpoint(tcp::v4(), port),
    options_values[max_sessions_param].as<std::size_t>(),
    options_values[recycled_sessions_param].as<std::size_t>(),
    options_values[listen_backlog_param].as<int>(),
    session_config);
} // create_session_manager_config

void print_config(std::ostream& stream, std::size_t cpu_count,
  const execution_config& the_execution_config,
  const server::session_manager_config& session_manager_config)
{
  const server::session_config& session_config = session_manager_config.managed_session_config;

  stream << "Number of found CPU(s)             : " << cpu_count                                           << std::endl
         << "Number of session manager's threads: " << the_execution_config.session_manager_thread_count   << std::endl
         << "Number of sessions' threads        : " << the_execution_config.session_thread_count           << std::endl
         << "Total number of work threads       : " 
            << the_execution_config.session_thread_count + the_execution_config.session_manager_thread_count << std::endl
         << "Server listen port                 : " << session_manager_config.accepting_endpoint.port()  << std::endl
         << "Server stop timeout (seconds)      : " << the_execution_config.stop_timeout.total_seconds() << std::endl
         << "Maximum number of active sessions  : " << session_manager_config.max_session_count            << std::endl
         << "Maximum number of recycled sessions: " << session_manager_config.recycled_session_count       << std::endl
         << "TCP listen backlog size            : " << session_manager_config.listen_backlog             << std::endl
         << "Size of session's buffer (bytes)   : " << session_config.buffer_size                        << std::endl;

  stream << "Size of session's socket receive buffer (bytes): ";
  print_param(stream, session_config.socket_recv_buffer_size, os_default_text);
  stream << std::endl;

  stream << "Size of session's socket send buffer (bytes)   : ";
  print_param(stream, session_config.socket_send_buffer_size, os_default_text);
  stream << std::endl;

  stream << "Session's socket Nagle algorithm is: ";
  print_param(stream, session_config.no_delay, os_default_text);
  stream << std::endl;      
} // print_config

void fill_options_description(boost::program_options::options_description& options_description,
  std::size_t cpu_count)
{
  options_description.add_options()
    (
      help_param, 
      "produce help message"
    )
    (
      port_param, 
      boost::program_options::value<unsigned short>(), 
      "set the TCP port number for incoming connections' listening"
    )
    (
      session_threads_param, 
      boost::program_options::value<std::size_t>()->default_value(cpu_count), 
      "set the number of sessions' threads"
    )    
    (
      stop_timeout_param, 
      boost::program_options::value<long>()->default_value(60), 
      "set the server stop timeout, at one's expiration server work will be terminated (seconds)"
    )
    (
      max_sessions_param, 
      boost::program_options::value<std::size_t>()->default_value(10000), 
      "set the maximum number of simultaneously active sessions"
    )
    (
      recycled_sessions_param, 
      boost::program_options::value<std::size_t>()->default_value(100), 
      "set the maximum number of pooled inactive sessions"
    )
    (
      listen_backlog_param, 
      boost::program_options::value<int>()->default_value(6), 
      "set the size of TCP listen backlog"
    )
    (
      buffer_size_param, 
      boost::program_options::value<std::size_t>()->default_value(1024),
      "set the session's buffer size (bytes)"
    )
    (
      socket_recv_buffer_size_param, 
      boost::program_options::value<int>(),
      "set the size of session's socket receive buffer (bytes)"
    )  
    (
      socket_send_buffer_size_param, 
      boost::program_options::value<int>(),
      "set the size of session's socket send buffer (bytes)"
    )
    (
      socket_no_delay_param, 
      boost::program_options::value<bool>(),
      "set TCP_NODELAY option of session's socket"
    );  
} // fill_options_description

void start_session_manager(const session_manager_wrapper_ptr& wrapped_session_manager)
{
  wrapped_session_manager->the_session_manager->async_start(
    ma::make_custom_alloc_handler(wrapped_session_manager->start_wait_allocator,
      boost::bind(handle_session_manager_start, wrapped_session_manager, _1)));
  wrapped_session_manager->state = session_manager_wrapper::start_in_progress;
} // start_session_manager

void wait_session_manager(const session_manager_wrapper_ptr& wrapped_session_manager)
{
  wrapped_session_manager->the_session_manager->async_wait(
    ma::make_custom_alloc_handler(wrapped_session_manager->start_wait_allocator,
      boost::bind(handle_session_manager_wait, wrapped_session_manager, _1)));
} // wait_session_manager

void stop_session_manager(const session_manager_wrapper_ptr& wrapped_session_manager)
{
  wrapped_session_manager->the_session_manager->async_stop(
    ma::make_custom_alloc_handler(wrapped_session_manager->stop_allocator,
      boost::bind(handle_session_manager_stop, wrapped_session_manager, _1)));
  wrapped_session_manager->state = session_manager_wrapper::stop_in_progress;
} // stop_session_manager

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
} // run_io_service

void handle_work_exception(const session_manager_wrapper_ptr& wrapped_session_manager)
{
  boost::unique_lock<boost::mutex> wrapper_lock(wrapped_session_manager->access_mutex);
  if (session_manager_wrapper::stopped != wrapped_session_manager->state)
  {
    wrapped_session_manager->state = session_manager_wrapper::stopped;  
    std::cout << "Terminating server work due to unexpected exception...\n";
    wrapper_lock.unlock();
    wrapped_session_manager->state_changed.notify_one();      
  }  
} // handle_work_exception

void handle_program_exit(const session_manager_wrapper_ptr& wrapped_session_manager)
{
  boost::unique_lock<boost::mutex> wrapper_lock(wrapped_session_manager->access_mutex);
  std::cout << "Program exit request detected.\n";
  if (session_manager_wrapper::stopped == wrapped_session_manager->state)
  {    
    std::cout << "Server has already stopped.\n";
    return;
  }
  if (session_manager_wrapper::stop_in_progress == wrapped_session_manager->state)
  {    
    wrapped_session_manager->state = session_manager_wrapper::stopped;
    std::cout << "Server is already stopping. Terminating server work...\n";
    wrapper_lock.unlock();
    wrapped_session_manager->state_changed.notify_one();       
    return;
  }  
  // Begin server stop
  stop_session_manager(wrapped_session_manager);
  wrapped_session_manager->stopped_by_user = true;
  std::cout << "Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
  wrapper_lock.unlock();    
  wrapped_session_manager->state_changed.notify_one();  
} // handle_program_exit

void handle_session_manager_start(const session_manager_wrapper_ptr& wrapped_session_manager, 
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> wrapper_lock(wrapped_session_manager->access_mutex);
  if (session_manager_wrapper::start_in_progress == wrapped_session_manager->state)
  {     
    if (error)
    {       
      wrapped_session_manager->state = session_manager_wrapper::stopped;
      std::cout << "Server can't start due to error: " << error.message() << '\n';
      wrapper_lock.unlock();
      wrapped_session_manager->state_changed.notify_one();
      return;
    }    
    wrapped_session_manager->state = session_manager_wrapper::started;
    wait_session_manager(wrapped_session_manager);
    std::cout << "Server has started.\n";
  }
} // handle_session_manager_start

void handle_session_manager_wait(const session_manager_wrapper_ptr& wrapped_session_manager, 
  const boost::system::error_code& error)
{
  boost::unique_lock<boost::mutex> wrapper_lock(wrapped_session_manager->access_mutex);
  if (session_manager_wrapper::started == wrapped_session_manager->state)
  {
    stop_session_manager(wrapped_session_manager);
    std::cout << "Server can't continue work due to error: " << error.message() << '\n'
              << "Server is stopping...\n";
    wrapper_lock.unlock();
    wrapped_session_manager->state_changed.notify_one();    
  }
} // handle_session_manager_wait

void handle_session_manager_stop(const session_manager_wrapper_ptr& wrapped_session_manager, 
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> wrapper_lock(wrapped_session_manager->access_mutex);
  if (session_manager_wrapper::stop_in_progress == wrapped_session_manager->state)
  {      
    wrapped_session_manager->state = session_manager_wrapper::stopped;
    std::cout << "Server has stopped.\n";    
    wrapper_lock.unlock();
    wrapped_session_manager->state_changed.notify_one();
  }
} // handle_session_manager_stop