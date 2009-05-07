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
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <console_controller.hpp>
#include <sync_ostream.hpp>
#include <ma/codecvt_cast.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/nmea/cycle_read_session.hpp>

typedef std::codecvt<wchar_t, char, mbstate_t> wcodecvt_type;
typedef ma::nmea::cycle_read_session<boost::asio::serial_port> session_type;
typedef session_type::message_type message_type;
typedef boost::shared_ptr<message_type> message_ptr;
typedef ma::sync_ostream<std::wostream> sync_ostream_type;

void handle_handshake(
  sync_ostream_type&,
  std::locale&,
  const wcodecvt_type&,
  session_type&, 
  ma::handler_allocator&, 
  const boost::system::error_code&);

void handle_shutdown(
  sync_ostream_type&,  
  session_type&,   
  const boost::system::error_code&);

void handle_read(
  sync_ostream_type&,
  std::locale&,
  const wcodecvt_type&,
  session_type&, 
  ma::handler_allocator&, 
  const message_ptr&, 
  const boost::system::error_code&);

void handle_console_close(
  sync_ostream_type&,
  session_type&);

int _tmain(int argc, _TCHAR* argv[])
{
  int exit_code = EXIT_SUCCESS;
  std::locale sys_locale("");

  if (argc != 2)
  {
    boost::filesystem::wpath app_path(argv[0]);
    std::wcout << L"Usage: \"" << app_path.leaf() << L"\" <com_port>\n";
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

    std::wstring device_name(argv[1]);
    session_type::size_type read_buffer_size(1024);
    session_type::read_capacity_type read_message_buffer_capacity(32);

    std::wcout << L"NMEA 0183 device name        : " << device_name << L"\n";
    std::wcout << L"Read buffer size             : " << read_buffer_size << L"\n";
    std::wcout << L"Read message buffer capacity : " << read_message_buffer_capacity << L"\n";

    const wcodecvt_type& wcodecvt(std::use_facet<wcodecvt_type>(sys_locale));
    std::string ansi_device_name(ma::codecvt_cast::out(device_name, wcodecvt));
    ma::handler_allocator handler_allocator;
        
    // Before io_service && console_controller
    sync_ostream_type sync_ostream(std::wcout);

    boost::asio::io_service io_service(concurrent_count);   
    session_type session(io_service, read_buffer_size, read_message_buffer_capacity, "$", "\x0a");    

    // Prepare the lower layer - open the serial port
    session.next_layer().open(ansi_device_name);        

    // Start async operation
    session.async_handshake(ma::make_custom_alloc_handler(handler_allocator, 
      boost::bind(&handle_handshake, boost::ref(sync_ostream), boost::ref(sys_locale), boost::cref(wcodecvt),
        boost::ref(session), boost::ref(handler_allocator), _1)));    

    // Setup console controller
    ma::console_controller console_controller(
      boost::bind(handle_console_close, boost::ref(sync_ostream), boost::ref(session)));        

	  sync_ostream << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

    boost::thread_group thread_group;
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      thread_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
    }
    thread_group.join_all();

    sync_ostream << L"IO threads stopped.\n";    
  }

  return exit_code;
}

void handle_console_close(
  sync_ostream_type& sync_ostream,
  session_type& session)
{
  sync_ostream << L"User console close detected.\nStarting shutdown operation...\n";    
      
  session.async_shutdown(boost::bind(handle_shutdown, 
    boost::ref(sync_ostream), boost::ref(session), _1));

  sync_ostream << L"Shutdown operation (by user console closure) started.\n";
}

void handle_handshake(
  sync_ostream_type& sync_ostream,
  std::locale& locale,
  const wcodecvt_type& wcodecvt,
  session_type& session, 
  ma::handler_allocator& handler_allocator, 
  const boost::system::error_code& error)
{  
  if (error)  
  {
    sync_ostream << L"Handshake unsuccessful.\n";      
  }
  else
  {    
    sync_ostream << L"Handshake successful. Starting read operation...\n";

    message_ptr message(new message_type());
    session.async_read(*message, ma::make_custom_alloc_handler(handler_allocator,
      boost::bind(handle_read, boost::ref(sync_ostream), boost::ref(locale), boost::cref(wcodecvt),
        boost::ref(session), boost::ref(handler_allocator), message, _1)));

    sync_ostream << L"Read operation started.\n";
  }  
}

void handle_shutdown(
  sync_ostream_type& sync_ostream,
  session_type& session,   
  const boost::system::error_code& error)
{   
  if (error)
  {
    sync_ostream << L"Shutdown unsuccessful.\n";
  }
  else
  {
    sync_ostream << L"Shutdown successful.\n";
  }
  
  if (boost::asio::error::operation_aborted != error)
  {
    // Close all session operations        
    sync_ostream << L"Closing session...\n";
    boost::system::error_code close_error;
    session.close(close_error);
    if (close_error)
    {
      sync_ostream << L"Session closed with an error.\n";
    }
    else
    {
      sync_ostream << L"Session closed successful.\n";
    }    
  }
}

void handle_read(
  sync_ostream_type& sync_ostream,
  std::locale& locale,
  const wcodecvt_type& wcodecvt,
  session_type& session, 
  ma::handler_allocator& handler_allocator, 
  const message_ptr& message, 
  const boost::system::error_code& error)
{  
  if (boost::asio::error::eof == error)  
  {
    sync_ostream << L"Input stream closed. But it\'s serial port so starting read operation again...\n";

    session.async_read(*message, ma::make_custom_alloc_handler(handler_allocator,
      boost::bind(handle_read, boost::ref(sync_ostream), boost::ref(locale), boost::cref(wcodecvt),
      boost::ref(session), boost::ref(handler_allocator), message, _1)));

    sync_ostream << L"Read operation started.\n";      
  }
  else if (error)  
  {      
    sync_ostream << L"Read unsuccessful. Starting shutdown operation...\n";      
    
    session.async_shutdown(ma::make_custom_alloc_handler(handler_allocator,
      boost::bind(handle_shutdown, boost::ref(sync_ostream), boost::ref(session), _1)));

    sync_ostream << L"Shutdown operation started.\n";
  }
  else
  {
    std::wstring log_message(ma::codecvt_cast::in(**message, wcodecvt));              
    sync_ostream << L"Read successful.\nMessage: " +
      log_message + L"\nStarting read operation...\n";      
    
    session.async_read(*message, ma::make_custom_alloc_handler(handler_allocator,
      boost::bind(handle_read, boost::ref(sync_ostream), boost::ref(locale), boost::cref(wcodecvt),
        boost::ref(session), boost::ref(handler_allocator), message, _1)));

    sync_ostream << L"Read operation started.\n";      
  }  
}
