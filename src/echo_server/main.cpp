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

struct server_proxy_type;
typedef boost::shared_ptr<server_proxy_type> server_proxy_ptr;
typedef boost::function<void (void)> exception_handler;

enum operation_state
{
  ready_to_start,
  in_progress,
  done
};

struct server_proxy_type : private boost::noncopyable
{  
  boost::mutex mutex;  
  ma::echo::server_ptr server;
  operation_state stop_state;  
  boost::condition_variable stop_state_changed;    

  explicit server_proxy_type(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    ma::echo::server::settings settings)
    : server(new ma::echo::server(io_service, session_io_service, settings))    
    , stop_state(ready_to_start)
  {
  }

  ~server_proxy_type()
  {
  }
}; // server_proxy_type

const boost::posix_time::time_duration stop_timeout = boost::posix_time::seconds(30);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const server_proxy_ptr&);

void handle_program_exit(const server_proxy_ptr&);

void server_started(const server_proxy_ptr&,
  const boost::system::error_code&);

void server_has_to_stop(const server_proxy_ptr&,
  const boost::system::error_code&);

void server_stopped(const server_proxy_ptr&,
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
    server_proxy_ptr server_proxy(
      new server_proxy_type(server_io_service, session_io_service,
        ma::echo::server::settings(endpoint)));
    
    std::wcout << L"Server is starting at port: " << listen_port << L"\n";              
    
    // Start the server
    server_proxy->server->async_start(
      boost::bind(server_started, server_proxy, _1));
    
    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(handle_program_exit, server_proxy));

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler work_exception_handler(
      boost::bind(handle_work_exception, server_proxy));

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
    boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);
    if (ready_to_start == server_proxy->stop_state)
    {
      server_proxy->stop_state_changed.wait(server_proxy_lock);
    }
    if (done != server_proxy->stop_state)
    {
      if (!server_proxy->stop_state_changed.timed_wait(server_proxy_lock, stop_timeout))      
      {
        std::wcout << L"Server stop timeout expiration. Terminating server work.\n";
        exit_code = EXIT_FAILURE;
      }
      server_proxy->stop_state = done;
    }
    server_proxy_lock.unlock();
        
    server_io_service.stop();
    session_io_service.stop();

    std::wcout << L"Waiting until all of the work threads will stop.\n";
    work_threads.join_all();
    std::wcout << L"Work threads have done. Process will close.\n";    
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

void handle_work_exception(const server_proxy_ptr& server_proxy)
{
  boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);  
  server_proxy->stop_state = done;  
  std::wcout << L"Terminating server work due to unexpected exception.\n";
  server_proxy_lock.unlock();
  server_proxy->stop_state_changed.notify_one();      
}

void handle_program_exit(const server_proxy_ptr& server_proxy)
{
  boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);
  std::wcout << L"Program exit request detected.\n";
  if (done == server_proxy->stop_state)
  {    
    std::wcout << L"Server has already done.\n";
  }
  else if (in_progress == server_proxy->stop_state)
  {    
    server_proxy->stop_state = done;
    std::wcout << L"Server was already stopping. Terminating server work.\n";
    server_proxy_lock.unlock();
    server_proxy->stop_state_changed.notify_one();       
  }
  else
  {    
    server_proxy->server->async_stop(
      boost::bind(server_stopped, server_proxy, _1));
    server_proxy->stop_state = in_progress;
    std::wcout << L"Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work.\n";
    server_proxy_lock.unlock();    
    server_proxy->stop_state_changed.notify_one();
  }  
}

void server_started(const server_proxy_ptr& server_proxy,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);
  if (ready_to_start == server_proxy->stop_state)
  {   
    if (error)
    {       
      server_proxy->stop_state = done;
      std::wcout << L"Server can't start due to error.\n";
      server_proxy_lock.unlock();
      server_proxy->stop_state_changed.notify_one();
    }
    else
    {
      server_proxy->server->async_wait(
        boost::bind(server_has_to_stop, server_proxy, _1));      
      std::wcout << L"Server has started.\n";
    }
  }
}

void server_has_to_stop(const server_proxy_ptr& server_proxy,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);
  if (ready_to_start == server_proxy->stop_state)
  {
    // Start server stop    
    server_proxy->server->async_stop(
      boost::bind(server_stopped, server_proxy, _1));    
    server_proxy->stop_state = in_progress;
    std::wcout << L"Server can't continue work due to error. Server is stopping.\n";
    server_proxy_lock.unlock();
    server_proxy->stop_state_changed.notify_one();    
  }
}

void server_stopped(const server_proxy_ptr& server_proxy,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_proxy_lock(server_proxy->mutex);
  if (ready_to_start == server_proxy->stop_state)
  {      
    server_proxy->stop_state = done;
    std::wcout << L"Server has stopped.\n";    
    server_proxy_lock.unlock();
    server_proxy->stop_state_changed.notify_one();
  }
}