//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <locale>
#include <iostream>
#include <utility>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <ma/codecvt_cast.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/nmea/cyclic_read_session.hpp>
#include <console_controller.hpp>

typedef ma::in_place_handler_allocator<64 * sizeof(std::size_t)> handler_allocator_type;
typedef std::codecvt<wchar_t, char, mbstate_t> wcodecvt_type;
typedef ma::nmea::cyclic_read_session session;
typedef ma::nmea::cyclic_read_session_ptr session_ptr;
typedef ma::nmea::message_ptr message_ptr;
typedef boost::shared_ptr<message_ptr> ptr_to_message_ptr;

void handle_start(
  std::locale&,
  const wcodecvt_type&,
  const session_ptr&, 
  handler_allocator_type&, 
  const boost::system::error_code&);

void handle_stop(const boost::system::error_code&);

void handle_read(  
  std::locale&,
  const wcodecvt_type&,
  const session_ptr&, 
  handler_allocator_type&, 
  const ptr_to_message_ptr&, 
  const boost::system::error_code&);

void handle_console_close(const session_ptr&);

int _tmain(int argc, _TCHAR* argv[])
{
  int exit_code = EXIT_SUCCESS;
  std::locale sys_locale("");

  if (2 > argc || 4 < argc)
  {
    boost::filesystem::wpath app_path(argv[0]);
    std::wcout << L"Usage: \"" << app_path.leaf() << L"\" <com_port> [<read_buffer_size> [<message_queue_size>] ]\n";
  }
  else
  {
    std::size_t cpu_count = boost::thread::hardware_concurrency();
    std::size_t concurrent_count = 2 > cpu_count ? 2 : cpu_count;
    std::size_t thread_count = 2;

    std::wcout << L"Number of found CPUs: " << cpu_count << L"\n"
               << L"Number of concurrent work threads: " << concurrent_count << L"\n"
               << L"Total number of work threads     : " << thread_count << L"\n";

    std::wstring device_name(argv[1]);
    std::size_t read_buffer_size = std::max<std::size_t>(1024, session::min_read_buffer_size);
    std::size_t message_queue_size = std::max<std::size_t>(64, session::min_message_queue_size);
    if (2 < argc)
    {
      read_buffer_size = boost::lexical_cast<std::size_t>(argv[2]);
      if (3 < argc)
      {
        message_queue_size = boost::lexical_cast<std::size_t>(argv[3]);
      }
    }    

    std::wcout << L"NMEA 0183 device name    : " << device_name << L"\n";
    std::wcout << L"Read buffer size in bytes: " << read_buffer_size << L"\n";
    std::wcout << L"Read buffer size in messages: " << message_queue_size << L"\n";

    const wcodecvt_type& wcodecvt(std::use_facet<wcodecvt_type>(sys_locale));
    std::string ansi_device_name(ma::codecvt_cast::out(device_name, wcodecvt));
    handler_allocator_type handler_allocator;
            
    boost::asio::io_service io_service(concurrent_count);   
    session_ptr session(new session(
      io_service, read_buffer_size, message_queue_size, "$", "\x0a"));

    // Prepare the lower layer - open the serial port
    session->serial_port().open(ansi_device_name);        

    // Start session
    session->async_start
    (
      ma::make_custom_alloc_handler
      (
        handler_allocator, 
        boost::bind
        (
          &handle_start,
          boost::ref(sys_locale), 
          boost::cref(wcodecvt),
          session, 
          boost::ref(handler_allocator), 
          _1
        )
      )
    );    

    // Setup console controller
    ma::console_controller console_controller(boost::bind(&handle_console_close, session));        
    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

    // Create work threads
    boost::thread_group thread_group;
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service));
    }
    thread_group.join_all();

    std::wcout << L"Work threads stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(const session_ptr& session)
{
  std::wcout << L"User console close detected.\nStarting stop operation...\n";
  session->async_stop(boost::bind(&handle_stop, _1));
  //session->get_io_service().stop();
}

void handle_start(  
  std::locale& locale,
  const wcodecvt_type& wcodecvt,
  const session_ptr& session, 
  handler_allocator_type& handler_allocator, 
  const boost::system::error_code& error)
{  
  if (error)  
  {
    std::wcout << L"Start unsuccessful.\n";      
  }
  else
  {    
    std::wcout << L"Session started successful. Starting read operation...\n";
    ptr_to_message_ptr message(new message_ptr());
    session->async_read
    (
      *message, 
      ma::make_custom_alloc_handler
      (
        handler_allocator,
        boost::bind
        (
          &handle_read,
          boost::ref(locale), 
          boost::cref(wcodecvt),
          session, 
          boost::ref(handler_allocator), 
          message, 
          _1
        )
      )
    );
  }  
}

void handle_stop(const boost::system::error_code& error)
{   
  if (error)
  {
    std::wcout << L"Stop unsuccessful.\n";
  }
  else
  {
    std::wcout << L"Stop successful.\n";
  }
}

void handle_read(
  std::locale& locale,
  const wcodecvt_type& wcodecvt,
  const session_ptr& session, 
  handler_allocator_type& handler_allocator, 
  const ptr_to_message_ptr& message, 
  const boost::system::error_code& error)
{  
  if (boost::asio::error::eof == error)  
  {
    std::wcout << L"Input stream closed. But it\'s serial port so starting read operation again...\n";
    session->async_read
    (
      *message, 
      ma::make_custom_alloc_handler
      (
        handler_allocator,
        boost::bind
        (
          &handle_read,
          boost::ref(locale), 
          boost::cref(wcodecvt),
          session, 
          boost::ref(handler_allocator), 
          message, 
          _1
        )
      )
    );
  }
  else if (error)  
  {      
    std::wcout << L"Read unsuccessful. Starting stop operation...\n";
    session->async_stop
    (
      ma::make_custom_alloc_handler
      (
        handler_allocator,
        boost::bind(&handle_stop, _1)
      )
    );
  }
  else
  {
    std::wstring log_message(ma::codecvt_cast::in(**message, wcodecvt));              
    std::wcout << L"Read successful.\nMessage: " + log_message + L"\nStarting read operation...\n";
    session->async_read
    (
      *message, 
      ma::make_custom_alloc_handler
      (
        handler_allocator,
        boost::bind
        (
          &handle_read,
          boost::ref(locale), 
          boost::cref(wcodecvt),
          session, 
          boost::ref(handler_allocator), 
          message, 
          _1
        )
      )
    );    
  }  
}
