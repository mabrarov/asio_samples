//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server.hpp>
#include <console_controller.hpp>

struct wrapped_server;
typedef boost::shared_ptr<wrapped_server> wrapped_server_ptr;
typedef boost::function<void (void)> exception_handler;

struct wrapped_server : private boost::noncopyable
{  
  boost::mutex mutex;  
  ma::echo::server_ptr server;
  bool stop_in_progress;
  bool stopped;
  boost::condition_variable state_changed;    

  explicit wrapped_server(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    ma::echo::server::settings settings)
    : server(new ma::echo::server(io_service, session_io_service, settings))    
    , stop_in_progress(false) 
    , stopped(false)
  {
  }
}; // wrapped_server

const boost::posix_time::time_duration stop_timeout = boost::posix_time::seconds(30);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const wrapped_server_ptr&);

void handle_program_exit(const wrapped_server_ptr&);

void server_started(const wrapped_server_ptr&,
  const boost::system::error_code&);

void server_has_to_stop(const wrapped_server_ptr&,
  const boost::system::error_code&);

void server_stopped(const wrapped_server_ptr&,
  const boost::system::error_code&);

int _tmain(int argc, _TCHAR* argv[])
{
  int exit_code = EXIT_SUCCESS;
  if (argc < 2)
  {
    boost::filesystem::wpath app_path(argv[0]);
    std::wcout << L"Usage: \"" << app_path.leaf() << L"\" port\n";
  }
  else
  {
    std::size_t cpu_count(boost::thread::hardware_concurrency());
    if (!cpu_count)
    {
      cpu_count = 1;
    }
    std::size_t session_thread_count = cpu_count;
    std::size_t session_manager_thread_count = 1;

    std::wcout << L"Number of found CPUs: " << cpu_count << L"\n"               
               << L"Number of session manager's threads: " << session_manager_thread_count << L"\n"
               << L"Number of sessions' threads        : " << session_thread_count << L"\n" 
               << L"Total number of work threads       : " << session_thread_count + session_manager_thread_count << L"\n";

    unsigned short listen_port = boost::lexical_cast<unsigned short>(argv[1]);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), listen_port);
    
    // Before server_io_service
    boost::asio::io_service session_io_service;
    // ... for the right destruction order
    boost::asio::io_service server_io_service;

    // Create server
    wrapped_server_ptr wrapped_server(
      new wrapped_server(server_io_service, session_io_service,
        ma::echo::server::settings(endpoint)));
    
    std::wcout << L"Server is starting at port: " << listen_port << L"\n";              
    
    // Start the server
    wrapped_server->server->async_start(
      boost::bind(server_started, wrapped_server, _1));    
    
    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(handle_program_exit, wrapped_server));

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler work_exception_handler(
      boost::bind(handle_work_exception, wrapped_server));

    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for server's io_service to prevent threads' stop
    boost::asio::io_service::work server_work(server_io_service);    

    // Create work threads for session operations
    boost::thread_group work_threads;    
    for (std::size_t i = 0; i != session_thread_count; ++i)
    {
      work_threads.create_thread(
        boost::bind(run_io_service, boost::ref(session_io_service), 
          work_exception_handler));
    }
    // Create work threads for server operations
    for (std::size_t i = 0; i != session_manager_thread_count; ++i)
    {
      work_threads.create_thread(
        boost::bind(run_io_service, boost::ref(server_io_service), 
          work_exception_handler));      
    }

    // Wait until server stops
    boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
    if (!wrapped_server->stop_in_progress && !wrapped_server->stopped)
    {
      wrapped_server->state_changed.wait(lock);
    }
    if (!wrapped_server->stopped)
    {
      if (!wrapped_server->state_changed.timed_wait(lock, stop_timeout))      
      {
        std::wcout << L"Server stop timeout expiration. Server work will be aborted.\n";
        exit_code = EXIT_FAILURE;
      }
    }
    lock.unlock();
    
    std::wcout << L"Aborting server work.\n";
    server_io_service.stop();
    session_io_service.stop();

    std::wcout << L"Waiting until all of the work threads will stop.\n";
    work_threads.join_all();
    std::wcout << L"Work threads have stopped. Process will close.\n";    
  }

  return exit_code;
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

void handle_work_exception(const wrapped_server_ptr& wrapped_server)
{
  boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
  std::wcout << L"Server work will be aborted due to unexpected exception.\n";
  wrapped_server->stop_in_progress = false;
  wrapped_server->stopped = true;  
  lock.unlock();
  wrapped_server->state_changed.notify_one();      
}

void handle_program_exit(const wrapped_server_ptr& wrapped_server)
{
  boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
  if (wrapped_server->stopped)
  {
    std::wcout << L"Program exit request detected. Server has already stopped.\n";
  }
  else if (wrapped_server->stop_in_progress)
  { 
    std::wcout << L"Program exit request detected. Server is already stopping. Server work will be aborted.\n";    
    wrapped_server->stop_in_progress = false;
    wrapped_server->stopped = true;    
    lock.unlock();
    wrapped_server->state_changed.notify_one();    
  }
  else
  {    
    std::wcout << L"Program exit request detected. Server is stopping. Press Ctrl+C (Ctrl+Break) to abort server work.\n";    
    // Start server stop
    wrapped_server->server->async_stop(
      boost::bind(server_stopped, wrapped_server, _1));        
    wrapped_server->stop_in_progress = true;    
    lock.unlock();
    wrapped_server->state_changed.notify_one();
  }  
}

void server_started(const wrapped_server_ptr& wrapped_server,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
  if (!wrapped_server->stop_in_progress && !wrapped_server->stopped)
  {   
    if (error)
    { 
      std::wcout << L"Server can't start due to error. Server has stopped.\n";            
      wrapped_server->stopped = true;
      lock.unlock();
      wrapped_server->state_changed.notify_one();
    }
    else
    {      
      std::wcout << L"Server has started.\n";
      wrapped_server->server->async_wait(
        boost::bind(server_has_to_stop, wrapped_server, _1));      
    }
  }
}

void server_has_to_stop(const wrapped_server_ptr& wrapped_server,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
  if (!wrapped_server->stop_in_progress && !wrapped_server->stopped)
  {
    // Start server stop
    std::wcout << L"Server can't continue its work due to error. Server is stopping.\n";
    wrapped_server->server->async_stop(
      boost::bind(server_stopped, wrapped_server, _1));    
    wrapped_server->stop_in_progress = true;    
    lock.unlock();
    wrapped_server->state_changed.notify_one();    
  }
}

void server_stopped(const wrapped_server_ptr& wrapped_server,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(wrapped_server->mutex);
  if (!wrapped_server->stopped && wrapped_server->stop_in_progress)
  {  
    std::wcout << L"Server has stopped.\n";    
    wrapped_server->stop_in_progress = false;
    wrapped_server->stopped = true;    
    lock.unlock();
    wrapped_server->state_changed.notify_one();
  }
}