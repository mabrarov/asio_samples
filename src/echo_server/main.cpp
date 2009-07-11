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
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server.hpp>
#include <console_controller.hpp>

typedef unsigned short port_type;
typedef ma::echo::server server_type;
typedef server_type::pointer server_ptr;
typedef server_type::endpoint_type endpoint_type;

void handler_server_start(
  const server_ptr&,
  const boost::system::error_code&);

void handler_server_serve(
  const server_ptr&,
  const boost::system::error_code&);

void handler_server_stop(
  const server_ptr&,
  const boost::system::error_code&);

void handle_console_close(
  boost::asio::io_service& session_io_service,
  boost::asio::io_service& server_io_service);

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
	  std::size_t concurrent_count = 1 == cpu_count ? 2 : cpu_count;
    std::size_t thread_count = concurrent_count;

    std::wcout << L"Found cpu(s)                 : " << cpu_count << L"\n"
               << L"Concurrent IO thread count   : " << concurrent_count << L"\n"
               << L"Total IO thread count        : " << thread_count << L"\n";    

    boost::asio::io_service session_io_service(concurrent_count);
    boost::asio::io_service server_io_service(1);

    //todo
    server_ptr server(new server_type(server_io_service, session_io_service));
    server->async_start(
      boost::bind(&handler_server_start, server, _1));
    
    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(&handle_console_close,
      boost::ref(session_io_service), 
      boost::ref(server_io_service)));        

    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

    boost::thread_group thread_group;
    // Create work threads for session operations
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &session_io_service));
    }

    // Create work thread for server operations
    thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &server_io_service));    

    // Wait until all work is done or all io_services are stopped
    thread_group.join_all();
    std::wcout << L"IO threads stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(
  boost::asio::io_service& session_io_service,
  boost::asio::io_service& server_io_service)
{
  std::wcout << L"User console close detected.\nStarting stop operation...\n";
  session_io_service.stop();
  server_io_service.stop();
}

void handler_server_start(
  const server_ptr& server,
  const boost::system::error_code& error)
{
  //todo
  if (!error)
  {
    server->async_serve(
      boost::bind(&handler_server_serve, server, _1));
  }
}

void handler_server_serve(
  const server_ptr& server,
  const boost::system::error_code&)
{
  //todo
  server->async_stop(
    boost::bind(&handler_server_stop, server, _1));  
}

void handler_server_stop(
  const server_ptr&,
  const boost::system::error_code&)
{
  //todo  
}