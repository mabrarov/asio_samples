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

typedef unsigned short port_type;
typedef ma::echo::server server_type;
typedef server_type::pointer server_ptr;
typedef server_type::endpoint_type endpoint_type;

struct server_environment
{
  boost::mutex mutex_;
  boost::condition_variable stop_condition_;
  server_ptr server_;
  boost::asio::deadline_timer stop_timer_;
  enum state_type
  {
    ready,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  } state_;  

  server_environment(
    boost::asio::io_service& server_io_service,
    boost::asio::io_service& session_io_service)
    : server_(new server_type(server_io_service, session_io_service))
    , stop_timer_(server_io_service)
    , state_(ready) 
  {
  }
}; // server_environment

typedef boost::shared_ptr<server_environment> server_environment_ptr;

void handle_server_start(
  const server_environment_ptr&,
  const boost::system::error_code&);

void handle_server_serve(
  const server_environment_ptr&,
  const boost::system::error_code&);

void handle_server_stop(
  const server_environment_ptr&,
  const boost::system::error_code&);

void handle_server_stop_timeout(
  const server_environment_ptr&,
  const boost::system::error_code&);

void handle_console_close(
  const server_environment_ptr&);

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

    std::wcout << L"Found cpu(s): " << cpu_count << L"\n"               
               << L"Total server thread count: " << session_thread_count + server_thread_count << L"\n";        
    
    boost::asio::io_service session_io_service(session_thread_count);
    boost::asio::io_service server_io_service(server_thread_count);        
    server_environment_ptr server_env(new server_environment(
      server_io_service, session_io_service));

    server_env->state_ = server_environment::start_in_progress;
    server_env->server_->async_start(
      boost::bind(&handle_server_start, server_env, _1));
    
    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(&handle_console_close,
      boost::ref(session_io_service), 
      boost::ref(server_io_service)));        

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

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

    //todo

    std::wcout << L"Waiting for server have to be shotted down.\n";
    boost::unique_lock<boost::mutex> server_lock(server_mutex);
    if (!have_to_shutdown_server)
    {
      shutdown_server_condition.wait(server_lock);      
    }
    std::wcout << L"Shutting down the server.\n";
    //todo
    //server->async_stop(....)
    server_lock.unlock();
        
    std::wcout << L"Waiting for server is shotted down (20 sec).\n";
    server_lock.lock();
    if (server_is_stopped)
    {
      std::wcout << L"Server was shotted down. Waiting for work threads are stopped.\n";
    }
    else
    {
      if (server_stopped_condition.timed_wait(server_lock, boost::posix_time::seconds(20)))
      {
        std::wcout << L"Server was shotted down. Waiting for work threads are stopped.\n";
      }
      else
      {
        std::wcout << L"Server shutdown timeout. Terminating IO operations and waiting for work threads are stopped.\n";
      }
    }    
    server_lock.unlock();

    server_io_service.stop();
    session_io_service.stop();

    // Wait until all work is done or all io_services are stopped
    thread_group.join_all();
    std::wcout << L"IO threads stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(
  const server_environment_ptr& server_env)
{
  boost::mutex::scoped_lock lock(server_env->mutex_);
  switch (server_env->state_)
  {
  case server_environment::ready:
    lock.unlock();
    std::wcout << L"User console close detected.\nThe server wasn't started yet.\n";
    break;  

  case server_environment::start_in_progress:
  case server_environment::started:
    server_env->state_ = server_environment::stop_in_progress;
    server_env->server_->async_stop(
      boost::bind(&handle_server_stop, server_env, _1));
    server_env->stop_timer_->expires_from_now(boost::posix_time::seconds(60));
    server_env->stop_timer_->async_wait(
      boost::bind(&handle_server_stop_timeout, server_env, _1));
    lock.unlock();
    std::wcout << L"User console close detected.\nStopping the server...\n";
    break;

  case server_environment::stopped:
    lock.unlock();
    std::wcout << L"User console close detected.\nThe server is already stopped.\n";
    break;
  }  
}

void handle_server_start(
  const server_environment_ptr& server_env,
  const boost::system::error_code& error)
{    
  if (error)
  {
    boost::mutex::scoped_lock lock(server_env->mutex_);
    server_env->state_ = server_environment::stopped;
    lock.unlock();
    server_env->stop_condition_.notify_all();    
  }
  else
  {    
    boost::mutex::scoped_lock lock(server_env->mutex_);
    server_env->state_ = server_environment::started;
    server_env->server_->async_serve(
      boost::bind(&handle_server_serve, server_env, _1));    
  }
}

void handle_server_serve(
  const server_environment_ptr& server_env,
  const boost::system::error_code&)
{
  boost::mutex::scoped_lock lock(server_env->mutex_);
  server_env->state_ = server_environment::stop_in_progress;
  server_env->server_->async_stop(
    boost::bind(&handle_server_stop, server_env, _1));
  server_env->stop_timer_->expires_from_now(boost::posix_time::seconds(60));
  server_env->stop_timer_->async_wait(
    boost::bind(&handle_server_stop_timeout, server_env, _1));
  lock.unlock();
  std::wcout << L"Server can't continue work due to internal state.\nStopping the server...\n";
}

void handle_server_stop(
  const server_environment_ptr& server_env,
  const boost::system::error_code&)
{
  boost::mutex::scoped_lock lock(server_env->mutex_);
  if (server_env->stop_timer_.cancel())
  {
    server_env->state_ = server_environment::stopped;
    lock.unlock();
    server_env->stop_condition_.notify_all();
  }
}

void handle_server_stop_timeout(
  const server_environment_ptr& server_env,
  const boost::system::error_code& error)
{
  if (boost::asio::error::operation_aborted != error)
  {
    boost::mutex::scoped_lock lock(server_env->mutex_);
    server_env->state_ = server_environment::stopped;
    lock.unlock();
    server_env->stop_condition_.notify_all();
  }
}