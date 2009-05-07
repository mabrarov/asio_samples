//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <list>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <console_controller.hpp>
#include <sync_ostream.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/session_manager.hpp>

typedef ma::sync_ostream<std::wostream> sync_ostream_type;
typedef unsigned short port_type;
typedef boost::asio::ip::tcp::endpoint endpoint_type;
typedef boost::asio::ip::tcp::socket stream_type;
typedef boost::asio::ip::tcp::acceptor acceptor_type;
typedef ma::echo::session_manager session_manager_type;

void handle_console_close(sync_ostream_type&);                          

int _tmain(int argc, _TCHAR* argv[])
{
  int exit_code = EXIT_SUCCESS;
  if (argc < 2)
  {
    boost::filesystem::wpath app_path(argv[0]);
    std::wcout << L"Usage: \"" << app_path.leaf() << L"\" port_1 [port_2 [port_3 .. [port_n] .. ]]\n";
  }
  else
  {
    std::size_t cpu_count(boost::thread::hardware_concurrency());
    if (!cpu_count)
    {
      cpu_count = 1;
    }
	  std::size_t concurrent_count = 1 == cpu_count ? 2 : cpu_count;
    std::size_t thread_count = concurrent_count + 1;

    std::wcout << L"Found cpu(s)                 : " << cpu_count << L"\n"
               << L"Concurrent IO thread count   : " << concurrent_count << L"\n"
               << L"Total IO thread count        : " << thread_count << L"\n";    

    std::list<endpoint_type> accepting_endpoints;
    for (int i = 1; i != argc; ++i)
    {
      port_type port(boost::lexical_cast<port_type>(argv[i]));
      endpoint_type new_endpoint(boost::asio::ip::tcp::v4(), port);           
      accepting_endpoints.push_back(new_endpoint);
      std::wcout << L"Accepting port               : " << port << L"\n";
    } 
        
    // Before io_service && console_controller
    sync_ostream_type sync_ostream(std::wcout);

    boost::asio::io_service session_io_service(concurrent_count);            
    boost::asio::io_service& session_manager_io_service(session_io_service);    

    // Session manager
    session_manager_type session_manager(session_manager_io_service);

    // Setup console controller
    ma::console_controller console_controller(boost::bind(handle_console_close, boost::ref(sync_ostream)));        

	  sync_ostream << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

    boost::thread_group thread_group;
    // Create work threads for session handling 
    // (and for session_factories handling too, and for session_manager too) 
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &session_io_service));
    }    

    // Wait until all work is done or all io_services are stopped
    thread_group.join_all();
    sync_ostream << L"IO threads stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(sync_ostream_type& sync_ostream)
{
  sync_ostream << L"User console close detected.\nStarting shutdown operation...\n";
}