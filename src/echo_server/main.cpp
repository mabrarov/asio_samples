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
#include <ma/handler_allocation.hpp>
#include <ma/echo/server.hpp>
#include <console_controller.hpp>

const boost::posix_time::time_duration stop_timeout = boost::posix_time::seconds(30);

typedef unsigned short port_type;
typedef ma::echo::server session_manager_type;
typedef session_manager_type::pointer session_manager_ptr;
typedef session_manager_type::endpoint_type endpoint_type;
typedef boost::function<void (void)> exception_handler_type;

struct server_environment_type
{
  boost::mutex mutex_;
  boost::condition_variable stop_condition_;
  session_manager_ptr session_manager_;
  boost::asio::deadline_timer stop_timer_;
  bool stop_in_progress_;
  bool stopped_;  

  server_environment_type(boost::asio::io_service& server_io_service,
    boost::asio::io_service& session_io_service)
    : session_manager_(new session_manager_type(server_io_service, session_io_service))
    , stop_timer_(server_io_service)
    , stop_in_progress_(false) 
    , stopped_(false)
  {
  }
}; // server_environment_type

typedef boost::shared_ptr<server_environment_type> server_environment_ptr;

void on_server_start(const server_environment_ptr&,
  const boost::system::error_code&);

void on_server_serve(const server_environment_ptr&,
  const boost::system::error_code&);

void on_server_stop(const server_environment_ptr&,
  const boost::system::error_code&);

void on_server_stop_timeout(const server_environment_ptr&,
  const boost::system::error_code&);

void on_console_close(const server_environment_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler_type);

void on_exception(const server_environment_ptr&);

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
    
    boost::asio::io_service session_io_service(session_thread_count);
    boost::asio::io_service session_manager_io_service(session_manager_thread_count);        
    server_environment_ptr server_environment(
      new server_environment_type(session_manager_io_service, session_io_service));
    
    // Start the server
    server_environment->session_manager_->async_start(boost::bind(&on_server_start, server_environment, _1));
    std::wcout << L"Server is starting.\n";
    
    // Setup console controller
    ma::console_controller console_controller(boost::bind(&on_console_close, server_environment));

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler_type exception_handler(boost::bind(&on_exception, server_environment));
    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for server's io_service to prevent threads' stop
    boost::asio::io_service::work server_work(session_manager_io_service);    
    boost::thread_group thread_group;
    // Create work threads for session operations
    for (std::size_t i = 0; i != session_thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&run_io_service, boost::ref(session_io_service), exception_handler));
    }
    // Create work threads for server operations
    for (std::size_t i = 0; i != session_manager_thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&run_io_service, boost::ref(session_manager_io_service), exception_handler));      
    }

    // Wait until server has stopped or aborted
    std::wcout << L"Waiting until the server will stop.\n";
    boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
    if (!server_environment->stopped_)
    {
      server_environment->stop_condition_.wait(lock);
    }
    lock.unlock();
    
    // Stop all IO services
    session_manager_io_service.stop();
    session_io_service.stop();

    std::wcout << L"Waiting until all of the work threads will stop.\n";
    thread_group.join_all();
    std::wcout << L"Work threads have stopped.\n";    
  }

  return exit_code;
}

void run_io_service(boost::asio::io_service& io_service, exception_handler_type exception_handler)
{
  try 
  {
    io_service.run();
  }
  catch (...)
  {
    exception_handler();
  }
}

void on_exception(const server_environment_ptr& server_environment)
{
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  server_environment->stopped_ = true;
  std::wcout << L"Server work is interrupted due to to unexpected exception.\n";
  lock.unlock();
  // Notify main thread
  server_environment->stop_condition_.notify_all();    
}

void on_console_close(const server_environment_ptr& server_environment)
{
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  if (server_environment->stop_in_progress_)
  {
    std::wcout << L"User console close detected. Server is already stopping.\n";
  }
  else if (server_environment->stopped_)
  {    
    std::wcout << L"User console close detected. Server has already stopped.\n";
  }
  else
  {
    // Start server stop
    server_environment->session_manager_->async_stop(
      boost::bind(&on_server_stop, server_environment, _1));
    // Set up stop timer
    server_environment->stop_timer_.expires_from_now(stop_timeout);
    server_environment->stop_timer_.async_wait(
      boost::bind(&on_server_stop_timeout, server_environment, _1));
    // Remember server state
    server_environment->stop_in_progress_ = true;   
    std::wcout << L"User console close detected. Server is stopping.\n";    
  }  
}

void on_server_start(const server_environment_ptr& server_environment,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  if (!server_environment->stop_in_progress_ && !server_environment->stopped_)
  {   
    if (error)
    {    
      // Remember server state
      server_environment->stopped_ = true;
      std::wcout << L"Server can't start due to error. Server has stopped.\n";
      lock.unlock();
      // Notify main thread
      server_environment->stop_condition_.notify_all();      
    }
    else
    {
      // Start waiting until server has done all the work
      server_environment->session_manager_->async_serve(
        boost::bind(&on_server_serve, server_environment, _1));
      std::wcout << L"Server has started.\n";
    }
  }  
}

void on_server_serve(const server_environment_ptr& server_environment,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  if (!server_environment->stop_in_progress_ && !server_environment->stopped_)
  {
    // Start server stop
    server_environment->session_manager_->async_stop(
      boost::bind(&on_server_stop, server_environment, _1));
    // Set up stop timer
    server_environment->stop_timer_.expires_from_now(stop_timeout);
    server_environment->stop_timer_.async_wait(
      boost::bind(&on_server_stop_timeout, server_environment, _1));
    // Remember server state
    server_environment->stop_in_progress_ = true;    
    std::wcout << L"Server can't continue its work due to error. Server is stopping.\n";
  }
}

void on_server_stop(const server_environment_ptr& server_environment,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  if (!server_environment->stopped_ && server_environment->stop_timer_.cancel())
  {  
    // Remember server state
    server_environment->stopped_ = true;
    std::wcout << L"Server has stopped.\n";
    lock.unlock();
    // Notify main thread
    server_environment->stop_condition_.notify_all();    
  }
}

void on_server_stop_timeout(const server_environment_ptr& server_environment,
  const boost::system::error_code& error)
{
  boost::unique_lock<boost::mutex> lock(server_environment->mutex_);
  if (!server_environment->stopped_ && !error)
  {    
    // Remember server state
    server_environment->stopped_ = true;
    std::wcout << L"Server work is interrupted because of the stop timeout expiration.\n";
    lock.unlock();
    // Notify main thread
    server_environment->stop_condition_.notify_all();    
  }
}