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
typedef ma::echo::server server_type;
typedef server_type::pointer server_ptr;
typedef server_type::endpoint_type endpoint_type;

struct server_env_type
{
  boost::mutex mutex_;
  boost::condition_variable stop_condition_;
  server_ptr server_;
  boost::asio::deadline_timer stop_timer_;
  bool stop_in_progress_;
  bool stopped_;  

  server_env_type(
    boost::asio::io_service& server_io_service,
    boost::asio::io_service& session_io_service)
    : server_(new server_type(server_io_service, session_io_service))
    , stop_timer_(server_io_service)
    , stop_in_progress_(false) 
    , stopped_(false)
  {
  }
}; // server_env_type

typedef boost::shared_ptr<server_env_type> server_env_ptr;

void handle_server_start(
  const server_env_ptr&,
  const boost::system::error_code&);

void handle_server_serve(
  const server_env_ptr&,
  const boost::system::error_code&);

void handle_server_stop(
  const server_env_ptr&,
  const boost::system::error_code&);

void handle_server_stop_timeout(
  const server_env_ptr&,
  const boost::system::error_code&);

void handle_console_close(
  const server_env_ptr&);

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
    std::size_t server_thread_count = 1;

    std::wcout << L"CPUs count        : " << cpu_count << L"\n"               
               << L"Work threads count: " << session_thread_count + server_thread_count << L"\n";        
    
    boost::asio::io_service session_io_service(session_thread_count);
    boost::asio::io_service server_io_service(server_thread_count);        
    server_env_ptr server_env(new server_env_type(server_io_service, session_io_service));
    
    // Start the server
    server_env->server_->async_start(
      boost::bind(&handle_server_start, server_env, _1));
    std::wcout << L"Server is starting.\n";
    
    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(&handle_console_close, server_env));

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for server's io_service to prevent threads' stop
    boost::asio::io_service::work server_work(server_io_service);
    boost::thread_group thread_group;
    // Create work threads for session operations
    for (std::size_t i = 0; i != session_thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &session_io_service));
    }
    // Create work threads for server operations
    for (std::size_t i = 0; i != server_thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &server_io_service));
    }

    // Wait until server has stopped or aborted
    std::wcout << L"Waiting until the server will stop.\n";
    boost::unique_lock<boost::mutex> lock(server_env->mutex_);
    if (!server_env->stopped_)
    {
      server_env->stop_condition_.wait(lock);
    }
    lock.unlock();
    
    // Stop all IO services
    server_io_service.stop();
    session_io_service.stop();

    std::wcout << L"Waiting until the work threads will stop.\n";
    thread_group.join_all();
    std::wcout << L"Work threads have stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(
  const server_env_ptr& server_env)
{
  boost::unique_lock<boost::mutex> lock(server_env->mutex_);
  if (server_env->stop_in_progress_)
  {
    std::wcout << L"User console close detected. Server is already stopping.\n";
  }
  else if (server_env->stopped_)
  {    
    std::wcout << L"User console close detected. Server has already stopped.\n";
  }
  else
  {
    // Start server stop
    server_env->server_->async_stop(
      boost::bind(&handle_server_stop, server_env, _1));
    // Set up stop timer
    server_env->stop_timer_.expires_from_now(stop_timeout);
    server_env->stop_timer_.async_wait(
      boost::bind(&handle_server_stop_timeout, server_env, _1));
    // Remember server state
    server_env->stop_in_progress_ = true;   
    std::wcout << L"User console close detected. Server is stopping.\n";    
  }  
}

void handle_server_start(
  const server_env_ptr& server_env,
  const boost::system::error_code& error)
{  
  boost::unique_lock<boost::mutex> lock(server_env->mutex_);
  if (!server_env->stop_in_progress_ && !server_env->stopped_)
  {   
    if (error)
    {    
      // Remember server state
      server_env->stopped_ = true;
      std::wcout << L"Server can't start due to error. Server stopped.\n";
      lock.unlock();
      // Notify main thread
      server_env->stop_condition_.notify_all();      
    }
    else
    {
      // Start waiting until server has done all the work
      server_env->server_->async_serve(
        boost::bind(&handle_server_serve, server_env, _1));
      std::wcout << L"Server started.\n";
    }
  }  
}

void handle_server_serve(
  const server_env_ptr& server_env,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(server_env->mutex_);
  if (!server_env->stop_in_progress_ && !server_env->stopped_)
  {
    // Start server stop
    server_env->server_->async_stop(
      boost::bind(&handle_server_stop, server_env, _1));
    // Set up stop timer
    server_env->stop_timer_.expires_from_now(stop_timeout);
    server_env->stop_timer_.async_wait(
      boost::bind(&handle_server_stop_timeout, server_env, _1));
    // Remember server state
    server_env->stop_in_progress_ = true;    
    std::wcout << L"Server can't continue to work due to error. Server is stopping.\n";
  }
}

void handle_server_stop(
  const server_env_ptr& server_env,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> lock(server_env->mutex_);
  if (!server_env->stopped_ && server_env->stop_timer_.cancel())
  {  
    // Remember server state
    server_env->stopped_ = true;
    std::wcout << L"Server stopped.\n";
    lock.unlock();
    // Notify main thread
    server_env->stop_condition_.notify_all();    
  }
}

void handle_server_stop_timeout(
  const server_env_ptr& server_env,
  const boost::system::error_code& error)
{
  boost::unique_lock<boost::mutex> lock(server_env->mutex_);
  if (!server_env->stopped_ && !error)
  {    
    // Remember server state
    server_env->stopped_ = true;
    std::wcout << L"Server work is interrupted because of the stop timeout expiration.\n";
    lock.unlock();
    // Notify main thread
    server_env->stop_condition_.notify_all();    
  }
}