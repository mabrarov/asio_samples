//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <boost/smart_ptr.hpp>
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

struct session_manager_wrapper : private boost::noncopyable
{  
  enum state_type
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  };

  volatile bool stopped_by_program_exit;
  volatile state_type state;
  boost::mutex access_mutex;  
  server::session_manager_ptr wrapped_session_manager;    
  boost::condition_variable state_changed;  
  ma::in_place_handler_allocator<128> start_wait_allocator;
  ma::in_place_handler_allocator<128> stop_allocator;

  explicit session_manager_wrapper(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    const server::session_manager_config& config)
    : stopped_by_program_exit(false)
    , state(ready_to_start)
    , wrapped_session_manager(boost::make_shared<server::session_manager>(
        boost::ref(io_service), boost::ref(session_io_service), config))        
  {
  }

  ~session_manager_wrapper()
  {
  }
}; // session_manager_wrapper

void fill_options_description(boost::program_options::options_description&);

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

server::session_config create_session_config(
  const boost::program_options::variables_map& options_values);

server::session_manager_config create_session_manager_config(
  const boost::program_options::variables_map& options_values,
  const server::session_config& session_config);

void print_config(std::ostream& stream, 
  const boost::posix_time::time_duration& stop_timeout,
  std::size_t cpu_num, std::size_t session_thread_num, std::size_t sm_thread_num,
  const server::session_manager_config& sm_config);

template <typename Value>
void print_param(std::ostream& stream, 
  const boost::optional<Value>& value, 
  const std::string& default_value)
{
  if (value) 
  {
    stream << *value;
  }
  else 
  {
    stream << default_value;
  }
}

template <>
void print_param<bool>(std::ostream& stream, 
  const boost::optional<bool>& value, 
  const std::string& default_value)
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
    stream << default_value;
  }
}

int _tmain(int argc, _TCHAR* argv[])
{   
  int exit_code = EXIT_SUCCESS;
  try 
  {    
    boost::program_options::options_description options_description("Allowed options");
    fill_options_description(options_description);

    boost::program_options::variables_map options_values;  
    boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), options_values);
    boost::program_options::notify(options_values);

    if (options_values.count(help_param))
    {
      std::cout << options_description;
    }
    else if (!options_values.count(port_param))
    {    
      exit_code = EXIT_FAILURE;
      std::cout << "Port must be specified" << std::endl << options_description;
    }
    else
    {        
      std::size_t cpu_num = boost::thread::hardware_concurrency();
      std::size_t session_thread_num = options_values[session_threads_param].as<std::size_t>();
      if (!session_thread_num) 
      {
        session_thread_num = cpu_num ? cpu_num : 1;
      }
      std::size_t sm_thread_num = 1;      
      boost::posix_time::time_duration stop_timeout = boost::posix_time::seconds(options_values[stop_timeout_param].as<long>());
      server::session_config session_config = create_session_config(options_values);
      server::session_manager_config sm_config = create_session_manager_config(options_values, session_config);

      print_config(std::cout, stop_timeout, cpu_num, session_thread_num, sm_thread_num, sm_config);
                  
      // Before sm_io_service
      boost::asio::io_service session_io_service(session_thread_num);      
      // ... for the right destruction order
      boost::asio::io_service sm_io_service(sm_thread_num);

      session_manager_wrapper_ptr sm_wrapper(boost::make_shared<session_manager_wrapper>(
        boost::ref(sm_io_service), boost::ref(session_io_service), sm_config));
      
      std::cout << "Server is starting...\n";
      
      // Start the server      
      boost::unique_lock<boost::mutex> sm_wrapper_lock(sm_wrapper->access_mutex);    
      start_session_manager(sm_wrapper);
      sm_wrapper_lock.unlock();
      
      // Setup console controller
      ma::console_controller console_controller(boost::bind(handle_program_exit, sm_wrapper));
      std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

      // Exception handler for automatic server aborting
      exception_handler work_exception_handler(boost::bind(handle_work_exception, sm_wrapper));

      // Create work for sessions' io_service to prevent threads' stop
      boost::asio::io_service::work session_work(session_io_service);
      // Create work for session_manager's io_service to prevent threads' stop
      boost::asio::io_service::work session_manager_work(sm_io_service);    

      // Create work threads for session operations
      boost::thread_group work_threads;    
      for (std::size_t i = 0; i != session_thread_num; ++i)
      {
        work_threads.create_thread(boost::bind(run_io_service, 
          boost::ref(session_io_service), work_exception_handler));
      }
      // Create work threads for session manager operations
      for (std::size_t i = 0; i != sm_thread_num; ++i)
      {
        work_threads.create_thread(boost::bind(run_io_service, 
          boost::ref(sm_io_service), work_exception_handler));      
      }

      // Wait until server stops
      sm_wrapper_lock.lock();
      while (session_manager_wrapper::stop_in_progress != sm_wrapper->state 
        && session_manager_wrapper::stopped != sm_wrapper->state)
      {
        sm_wrapper->state_changed.wait(sm_wrapper_lock);
      }
      if (session_manager_wrapper::stopped != sm_wrapper->state)
      {
        if (!sm_wrapper->state_changed.timed_wait(sm_wrapper_lock, stop_timeout))      
        {
          std::cout << "Server stop timeout expiration. Terminating server work...\n";
          exit_code = EXIT_FAILURE;
        }
        sm_wrapper->state = session_manager_wrapper::stopped;
      }
      if (!sm_wrapper->stopped_by_program_exit)
      {
        exit_code = EXIT_FAILURE;
      }
      sm_wrapper_lock.unlock();
          
      sm_io_service.stop();
      session_io_service.stop();

      std::cout << "Server work was terminated. Waiting until all of the work threads will stop...\n";
      work_threads.join_all();
      std::cout << "Work threads have stopped. Process will close." << std::endl;    
    }
  }
  catch (const boost::program_options::error& e)
  {
    exit_code = EXIT_FAILURE;
    std::cerr << "Error reading options: " << e.what() << std::endl;      
  }
  catch (const std::exception& e)
  {
    exit_code = EXIT_FAILURE;
    std::cerr << "Unexpected exception: " << e.what() << std::endl;      
  }
  catch (...)
  {
    exit_code = EXIT_FAILURE;
    std::cerr << "Unknown exception" << std::endl;
  }
  return exit_code;
}

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
}

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
}

void print_config(std::ostream& stream, 
  const boost::posix_time::time_duration& stop_timeout,
  std::size_t cpu_num, std::size_t session_thread_num, std::size_t sm_thread_num,
  const server::session_manager_config& sm_config)
{
  const server::session_config& session_config = sm_config.managed_session_config;

  stream << "Number of found CPU(s)             : " << cpu_num                      << std::endl
         << "Number of session manager's threads: " << sm_thread_num                << std::endl
         << "Number of sessions' threads        : " << session_thread_num           << std::endl
         << "Total number of work threads       : " << session_thread_num + sm_thread_num << std::endl
         << "Server listen port                 : " << sm_config.accepting_endpoint.port()   << std::endl
         << "Server stop timeout (seconds)      : " << stop_timeout.total_seconds() << std::endl
         << "Maximum number of active sessions  : " << sm_config.max_session_num      << std::endl
         << "Maximum number of recycled sessions: " << sm_config.recycled_session_num << std::endl
         << "TCP listen backlog size            : " << sm_config.listen_backlog    << std::endl
         << "Size of session's buffer (bytes)   : " << session_config.buffer_size  << std::endl;

  stream << "Size of session's socket receive buffer (bytes): ";
  print_param(stream, session_config.socket_recv_buffer_size, os_default_text);
  stream << std::endl;

  stream << "Size of session's socket send buffer (bytes)   : ";
  print_param(stream, session_config.socket_send_buffer_size, os_default_text);
  stream << std::endl;

  stream << "Session's socket Nagle algorithm is: ";
  print_param(stream, session_config.no_delay, os_default_text);
  stream << std::endl;      
}

void fill_options_description(boost::program_options::options_description& options_description)
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
      boost::program_options::value<std::size_t>()->default_value(0), 
      "set the number of sessions' threads, zero means set it equal to the number of CPUs"
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
}

void start_session_manager(const session_manager_wrapper_ptr& sm_wrapper)
{
  sm_wrapper->wrapped_session_manager->async_start(
    ma::make_custom_alloc_handler(sm_wrapper->start_wait_allocator,
      boost::bind(handle_session_manager_start, sm_wrapper, _1)));
  sm_wrapper->state = session_manager_wrapper::start_in_progress;
}

void wait_session_manager(const session_manager_wrapper_ptr& sm_wrapper)
{
  sm_wrapper->wrapped_session_manager->async_wait(
    ma::make_custom_alloc_handler(sm_wrapper->start_wait_allocator,
      boost::bind(handle_session_manager_wait, sm_wrapper, _1)));
}

void stop_session_manager(const session_manager_wrapper_ptr& sm_wrapper)
{
  sm_wrapper->wrapped_session_manager->async_stop(
    ma::make_custom_alloc_handler(sm_wrapper->stop_allocator,
      boost::bind(handle_session_manager_stop, sm_wrapper, _1)));
  sm_wrapper->state = session_manager_wrapper::stop_in_progress;
}

void run_io_service(boost::asio::io_service& io_service, 
  exception_handler io_service_exception_handler)
{
  try 
  {
    io_service.run();
  }
  catch (...)
  {
    io_service_exception_handler();
  }
}

void handle_work_exception(const session_manager_wrapper_ptr& sm_wrapper)
{
  boost::unique_lock<boost::mutex> wrapper_lock(sm_wrapper->access_mutex);
  if (session_manager_wrapper::stopped != sm_wrapper->state)
  {
    sm_wrapper->state = session_manager_wrapper::stopped;  
    std::cout << "Terminating server work due to unexpected exception...\n";
    wrapper_lock.unlock();
    sm_wrapper->state_changed.notify_one();      
  }  
}

void handle_program_exit(const session_manager_wrapper_ptr& sm_wrapper)
{
  boost::unique_lock<boost::mutex> wrapper_lock(sm_wrapper->access_mutex);
  std::cout << "Program exit request detected.\n";
  if (session_manager_wrapper::stopped == sm_wrapper->state)
  {    
    std::cout << "Server has already stopped.\n";
  }
  else if (session_manager_wrapper::stop_in_progress == sm_wrapper->state)
  {    
    sm_wrapper->state = session_manager_wrapper::stopped;
    std::cout << "Server is already stopping. Terminating server work...\n";
    wrapper_lock.unlock();
    sm_wrapper->state_changed.notify_one();       
  }
  else
  {    
    // Start server stop
    stop_session_manager(sm_wrapper);
    sm_wrapper->stopped_by_program_exit = true;
    std::cout << "Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
    wrapper_lock.unlock();    
    sm_wrapper->state_changed.notify_one();
  }  
}

void handle_session_manager_start(const session_manager_wrapper_ptr& sm_wrapper, 
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> wrapper_lock(sm_wrapper->access_mutex);
  if (session_manager_wrapper::start_in_progress == sm_wrapper->state)
  {   
    if (error)
    {       
      sm_wrapper->state = session_manager_wrapper::stopped;
      std::cout << "Server can't start due to error.\n";
      wrapper_lock.unlock();
      sm_wrapper->state_changed.notify_one();
    }
    else
    {
      sm_wrapper->state = session_manager_wrapper::started;
      wait_session_manager(sm_wrapper);
      std::cout << "Server has started.\n";
    }
  }
}

void handle_session_manager_wait(const session_manager_wrapper_ptr& sm_wrapper, 
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> wrapper_lock(sm_wrapper->access_mutex);
  if (session_manager_wrapper::started == sm_wrapper->state)
  {
    stop_session_manager(sm_wrapper);
    std::cout << "Server can't continue work due to error. Server is stopping...\n";
    wrapper_lock.unlock();
    sm_wrapper->state_changed.notify_one();    
  }
}

void handle_session_manager_stop(const session_manager_wrapper_ptr& sm_wrapper, 
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> wrapper_lock(sm_wrapper->access_mutex);
  if (session_manager_wrapper::stop_in_progress == sm_wrapper->state)
  {      
    sm_wrapper->state = session_manager_wrapper::stopped;
    std::cout << "Server has stopped.\n";    
    wrapper_lock.unlock();
    sm_wrapper->state_changed.notify_one();
  }
}