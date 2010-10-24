//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <cstdlib>
#include <locale>
#include <iostream>
#include <utility>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/lexical_cast.hpp>
#include <ma/codecvt_cast.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/nmea/frame.hpp>
#include <ma/nmea/cyclic_read_session.hpp>
#include <ma/console_controller.hpp>

typedef ma::in_place_handler_allocator<128> handler_allocator_type;
typedef std::codecvt<wchar_t, char, mbstate_t> wcodecvt_type;
typedef ma::nmea::cyclic_read_session     session;
typedef ma::nmea::cyclic_read_session_ptr session_ptr;
typedef ma::nmea::frame_ptr               frame_ptr;

void handle_start(std::locale&, const wcodecvt_type&, const session_ptr&, 
  handler_allocator_type&, const boost::system::error_code&);

void handle_stop(const boost::system::error_code&);

void handle_read(std::locale&, const wcodecvt_type&, const session_ptr&, 
  handler_allocator_type&, const boost::system::error_code&, const frame_ptr&);

void handle_console_close(const session_ptr&);

void print_usage(int argc, _TCHAR* argv[])
{
  std::wstring file_name;
  if (argc > 0)
  {
    boost::filesystem::wpath app_path(argv[0]);
    file_name = app_path.leaf();
  }
  else
  {
    file_name = L"nmea_client.exe";
  }
  std::wcout << L"Usage: \"" << file_name << L"\" <com_port> [<read_buffer_size> [<message_queue_size>] ]" << std::endl;  
}

//typedef boost::shared_ptr<handler_allocator_type> allocator_ptr;
//void handle_write(const allocator_ptr&, const boost::system::error_code&, const frame_ptr&)
//{
//  //todo
//}

int _tmain(int argc, _TCHAR* argv[])
{
  try
  {  
    std::locale console_locale("Russian_Russia.866");
    std::locale sys_locale("");
    std::wcout.imbue(console_locale);
    std::wcerr.imbue(console_locale);

    if (2 > argc || 4 < argc)
    {
      print_usage(argc, argv);
      return EXIT_FAILURE;
    }

    std::size_t cpu_num = boost::thread::hardware_concurrency();
    std::size_t concurrent_num = 2 > cpu_num ? 2 : cpu_num;
    std::size_t thread_num = 2;

    std::wcout << L"Number of found CPUs             : " << cpu_num        << std::endl
               << L"Number of concurrent work threads: " << concurrent_num << std::endl
               << L"Total number of work threads     : " << thread_num     << std::endl;

    std::wstring device_name(argv[1]);
    std::size_t read_buffer_size = std::max<std::size_t>(1024, session::min_read_buffer_size);
    std::size_t message_queue_size = std::max<std::size_t>(64, session::min_message_queue_size);

    if (argc > 2)
    {
      try 
      {      
        read_buffer_size = boost::lexical_cast<std::size_t>(argv[2]);
        if (3 < argc)
        {
          message_queue_size = boost::lexical_cast<std::size_t>(argv[3]);
        }
      }
      catch (const boost::bad_lexical_cast& e)
      {
        std::cerr << L"Invalid parameter value/format: " << e.what() << std::endl;
        print_usage(argc, argv);
        return EXIT_FAILURE;
      }
      catch (const std::exception& e)
      {    
        std::cerr << "Unexpected error during parameters parsing: " << e.what() << std::endl;
        print_usage(argc, argv);
        return EXIT_FAILURE;
      } 
    } // if (argc > 2)

    std::wcout << L"NMEA 0183 device serial port: " << device_name        << std::endl
               << L"Read buffer size (bytes)    : " << read_buffer_size   << std::endl
               << L"Read buffer size (messages) : " << message_queue_size << std::endl;  

    const wcodecvt_type& wcodecvt(std::use_facet<wcodecvt_type>(sys_locale));
    std::string ansi_device_name(ma::codecvt_cast::out(device_name, wcodecvt));
    handler_allocator_type in_place_handler_allocator;
            
    // An io_service for the thread pool (for the executors... Java Executors API? Apache MINA :)
    boost::asio::io_service io_service(concurrent_num);   
    session_ptr session(boost::make_shared<session>(boost::ref(io_service), 
      read_buffer_size, message_queue_size, "$", "\x0a"));

    // Prepare the lower layer - open the serial port  
    boost::system::error_code error;
    session->serial_port().open(ansi_device_name, error);
    if (error)
    {
      std::wstring error_message = ma::codecvt_cast::in(error.message(), wcodecvt);
      std::wcerr << L"Failed to open serial port: " << error_message << std::endl;
      return EXIT_FAILURE;
    }

    // Start session (not actually, because there are no work threads yet)
    session->async_start(ma::make_custom_alloc_handler(in_place_handler_allocator, 
      boost::bind(&handle_start, boost::ref(sys_locale), boost::cref(wcodecvt), 
        session, boost::ref(in_place_handler_allocator), _1)));    

    // Setup console controller
    ma::console_controller console_controller(boost::bind(&handle_console_close, session));        
    std::wcout << L"Press Ctrl+C (Ctrl+Break) to exit...\n";    

    // Create work threads
    boost::thread_group thread_group;
    for (std::size_t i = 0; i != thread_num; ++i)
    {
      thread_group.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service));
    }
    thread_group.join_all();

    std::wcout << L"Work threads stopped." << std::endl;      
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}

void handle_console_close(const session_ptr& session)
{
  std::wcout << L"User console close detected.\nStarting stop operation...\n";
  session->async_stop(boost::bind(&handle_stop, _1));
  //session->get_io_service().stop();
}

void handle_start(std::locale& locale, const wcodecvt_type& wcodecvt, const session_ptr& session, 
  handler_allocator_type& in_place_handler_allocator, const boost::system::error_code& error)
{  
  if (error)
  {
    std::wcout << L"Start unsuccessful.\n"; 
    return;
  }  
  std::wcout << L"Session started successful. Starting read operation...\n";    
  session->async_read(ma::make_custom_alloc_handler(in_place_handler_allocator,
    boost::bind(&handle_read, boost::ref(locale), boost::cref(wcodecvt),
      session, boost::ref(in_place_handler_allocator), _1, _2)));

  //allocator_ptr allocator = boost::make_shared<handler_allocator_type>();
  //frame_ptr nmea_frame = boost::make_shared<ma::nmea::message_type>("Bla..bla..bla");
  //session->async_write(boost::asio::buffer(*nmea_frame), ma::make_custom_alloc_handler(*allocator,
  //  boost::bind(&handle_write, allocator, _1, nmea_frame)));  
}

void handle_stop(const boost::system::error_code& error)
{   
  if (error)
  {
    std::wcout << L"Stop unsuccessful.\n";
    return;
  }
  std::wcout << L"Stop successful.\n";  
}

void handle_read(std::locale& locale, const wcodecvt_type& wcodecvt, const session_ptr& session, 
  handler_allocator_type& in_place_handler_allocator, const boost::system::error_code& error,
  const frame_ptr& nmea_frame)
{  
  if (boost::asio::error::eof == error)  
  {
    std::wcout << L"Input stream closed. But it\'s serial port so starting read operation again...\n";
    session->async_read(ma::make_custom_alloc_handler(in_place_handler_allocator,
      boost::bind(&handle_read, boost::ref(locale), boost::cref(wcodecvt), session, 
        boost::ref(in_place_handler_allocator), _1, _2)));
    return;
  }
  if (error)  
  {      
    std::wcout << L"Read unsuccessful. Starting stop operation...\n";
    session->async_stop(ma::make_custom_alloc_handler(in_place_handler_allocator,
      boost::bind(&handle_stop, _1)));
    return;
  }  
  std::wstring log_message(ma::codecvt_cast::in(*nmea_frame, wcodecvt));              
  std::wcout << L"Read successful.\nMessage: " + log_message + L"\nStarting read operation...\n";
  session->async_read(ma::make_custom_alloc_handler(in_place_handler_allocator,
    boost::bind(&handle_read, boost::ref(locale), boost::cref(wcodecvt), session, 
      boost::ref(in_place_handler_allocator), _1, _2)));  
}
