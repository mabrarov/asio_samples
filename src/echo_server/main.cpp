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

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/condition_variable.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_controller.hpp>
#include <ma/echo/server/session_manager.hpp>
#include "config.hpp"

namespace {

struct  session_manager_data;
typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
typedef boost::function<void (void)> exception_handler;  

struct session_manager_data : private boost::noncopyable
{
  typedef boost::mutex mutex_type;

  enum state_type
  {
    ready,
    start,
    work,
    stop,
    stopped
  }; // enum state_type

  mutex_type                mutex;
  volatile bool             stopped_by_user;
  volatile state_type       state;
  boost::condition_variable state_changed;
  ma::echo::server::session_manager_ptr session_manager;
  ma::in_place_handler_allocator<128>   start_wait_allocator;
  ma::in_place_handler_allocator<128>   stop_allocator;

  session_manager_data(boost::asio::io_service& io_service,
      boost::asio::io_service& session_io_service,
      const ma::echo::server::session_manager_config& config)
    : stopped_by_user(false)
    , state(ready)
    , session_manager(boost::make_shared<ma::echo::server::session_manager>(
          boost::ref(io_service), boost::ref(session_io_service), config))
  {
  }

  ~session_manager_data()
  {
  }
}; // session_manager_data

int run_server(const execution_config&, 
    const ma::echo::server::session_manager_config&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_manager_data_ptr&);
void handle_program_exit(const session_manager_data_ptr&);

void handle_session_manager_start(const session_manager_data_ptr&, 
    const boost::system::error_code&);
void handle_session_manager_wait(const session_manager_data_ptr&, 
    const boost::system::error_code&);
void handle_session_manager_stop(const session_manager_data_ptr&, 
    const boost::system::error_code&);

void start_session_manager(const session_manager_data_ptr&);
void wait_session_manager(const session_manager_data_ptr&);
void stop_session_manager(const session_manager_data_ptr&);

int run_server(const execution_config& the_execution_config,
    const ma::echo::server::session_manager_config& session_manager_config)
{
  // Before session_manager_io_service
  boost::asio::io_service session_io_service(
      the_execution_config.session_thread_count);

  // ... for the right destruction order
  boost::asio::io_service session_manager_io_service(
      the_execution_config.session_manager_thread_count);

  session_manager_data_ptr the_session_manager_data(
      boost::make_shared<session_manager_data>(
          boost::ref(session_manager_io_service), 
          boost::ref(session_io_service), session_manager_config));
      
  std::cout << "Server is starting...\n";
      
  // Start the server      
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);    
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
      i != the_execution_config.session_thread_count; ++i)
  {
    work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_io_service), work_exception_handler));
  }

  // Create work threads for session manager operations
  for (std::size_t i = 0; 
      i != the_execution_config.session_manager_thread_count; ++i)
  {
    work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_manager_io_service), work_exception_handler));
  }

  int exit_code = EXIT_SUCCESS;

  // Wait until server begins stopping or stops
  session_manager_lock.lock();

  while ((session_manager_data::stop != the_session_manager_data->state)
      && (session_manager_data::stopped != the_session_manager_data->state))
  {
    the_session_manager_data->state_changed.wait(session_manager_lock);
  }

  if (session_manager_data::stopped != the_session_manager_data->state)
  {
    // Wait until server stops with timeout
    if (!the_session_manager_data->state_changed.timed_wait(
        session_manager_lock, the_execution_config.stop_timeout))
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
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);

  if (session_manager_data::stopped != the_session_manager_data->state)
  {
    the_session_manager_data->state = session_manager_data::stopped;  

    std::cout << "Terminating server work due to unexpected exception...\n";

    session_manager_lock.unlock();
    the_session_manager_data->state_changed.notify_one();
  }
}

void handle_program_exit(
    const session_manager_data_ptr& the_session_manager_data)
{
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);

  std::cout << "Program exit request detected.\n";

  if (session_manager_data::stopped == the_session_manager_data->state)
  {    
    std::cout << "Server has already stopped.\n";
    return;
  }

  if (session_manager_data::stop == the_session_manager_data->state)
  {    
    the_session_manager_data->state = session_manager_data::stopped;

    std::cout << "Server is already stopping. Terminating server work...\n";

    session_manager_lock.unlock();
    the_session_manager_data->state_changed.notify_one();
    return;
  } 

  // Begin server stop
  stop_session_manager(the_session_manager_data);
  the_session_manager_data->stopped_by_user = true;

  std::cout << "Server is stopping." \
      " Press Ctrl+C (Ctrl+Break) to terminate server work...\n";

  session_manager_lock.unlock();
  the_session_manager_data->state_changed.notify_one();
}

void handle_session_manager_start(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
{   
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);

  if (session_manager_data::start == the_session_manager_data->state)
  {     
    if (error)
    {       
      the_session_manager_data->state = session_manager_data::stopped;

      std::cout << "Server can't start due to error: " 
          << error.message() << '\n';

      session_manager_lock.unlock();
      the_session_manager_data->state_changed.notify_one();
      return;
    }

    the_session_manager_data->state = session_manager_data::work;
    wait_session_manager(the_session_manager_data);
    std::cout << "Server has started.\n";
  }
}

void handle_session_manager_wait(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
{
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);

  if (session_manager_data::work == the_session_manager_data->state)
  {
    stop_session_manager(the_session_manager_data);

    std::cout << "Server can't continue work due to error: " 
        << error.message() << '\n' 
        << "Server is stopping...\n";

    session_manager_lock.unlock();
    the_session_manager_data->state_changed.notify_one();    
  }
}

void handle_session_manager_stop(
    const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code&)
{
  boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(
      the_session_manager_data->mutex);

  if (session_manager_data::stop == the_session_manager_data->state)
  {
    the_session_manager_data->state = session_manager_data::stopped;

    std::cout << "Server has stopped.\n";

    session_manager_lock.unlock();
    the_session_manager_data->state_changed.notify_one();
  }
}

void start_session_manager(
    const session_manager_data_ptr& the_session_manager_data)
{
  the_session_manager_data->session_manager->async_start(
      ma::make_custom_alloc_handler(
          the_session_manager_data->start_wait_allocator, boost::bind(
              handle_session_manager_start, the_session_manager_data, _1)));

  the_session_manager_data->state = session_manager_data::start;
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

  the_session_manager_data->state = session_manager_data::stop;
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
    // Parse user defined command line options
    std::size_t cpu_count = boost::thread::hardware_concurrency();

    const boost::program_options::options_description 
        cmd_options_description = build_cmd_options_description(cpu_count);
    
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

    // Parse configuration
    const execution_config the_execution_config =
        build_execution_config(cmd_options);
    const ma::echo::server::session_config session_config = 
        build_session_config(cmd_options);
    const ma::echo::server::session_manager_config session_manager_config =
        build_session_manager_config(cmd_options, session_config);

    // Show server configuration
    print_config(std::cout, cpu_count, the_execution_config, 
        session_manager_config);

    // Do the work
    return run_server(the_execution_config, session_manager_config);
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
