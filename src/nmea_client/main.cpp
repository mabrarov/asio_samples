//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <locale>
#include <vector>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <exception>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <ma/codecvt_cast.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/nmea/frame.hpp>
#include <ma/nmea/cyclic_read_session.hpp>
#include <ma/console_controller.hpp>

typedef std::codecvt<wchar_t, char, mbstate_t> wcodecvt_type;
typedef ma::nmea::cyclic_read_session        session;
typedef ma::nmea::cyclic_read_session_ptr    session_ptr;
typedef ma::nmea::frame_ptr                  frame_ptr;
typedef ma::in_place_handler_allocator<128>  handler_allocator_type;
typedef std::vector<frame_ptr>               frame_buffer_type;
typedef boost::shared_ptr<frame_buffer_type> frame_buffer_ptr;

void handle_start(const session_ptr& the_session,
    handler_allocator_type& the_allocator,
    const frame_buffer_ptr& frame_buffer,
    const boost::system::error_code& error);

void handle_stop(const boost::system::error_code& error);

void handle_read(const session_ptr& the_session,
    handler_allocator_type& the_allocator,
    const frame_buffer_ptr& frame_buffer,
    const boost::system::error_code& error,
    std::size_t frames_transferred);

void handle_console_close(const session_ptr&);

void print_frames(const frame_buffer_type& frames, std::size_t size);

void print_usage();

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try
  {
#if defined(WIN32)
    std::locale console_locale("Russian_Russia.866");
    std::wcout.imbue(console_locale);
    std::wcerr.imbue(console_locale);
    std::locale sys_locale("");
#endif

    if (2 > argc || 4 < argc)
    {
      print_usage();
      return EXIT_FAILURE;
    }

    std::size_t cpu_count = boost::thread::hardware_concurrency();
    std::size_t concurrent_count = 2 > cpu_count ? 2 : cpu_count;
    std::size_t thread_count = 2;

    std::cout << "Number of found CPUs             : "
              << cpu_count << std::endl
              << "Number of concurrent work threads: "
              << concurrent_count << std::endl
              << "Total number of work threads     : "
              << thread_count << std::endl;

#if defined(WIN32)
    std::wstring wide_device_name(argv[1]);
    const wcodecvt_type& wcodecvt(std::use_facet<wcodecvt_type>(sys_locale));
    std::string device_name(ma::codecvt_cast::out(wide_device_name, wcodecvt));
#else
    std::string device_name(argv[1]);
#endif

    std::size_t read_buffer_size =
        std::max<std::size_t>(1024, session::min_read_buffer_size);
    std::size_t message_queue_size =
        std::max<std::size_t>(64, session::min_message_queue_size);

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
        std::cerr << L"Invalid parameter value/format: "
            << e.what() << std::endl;
        print_usage();
        return EXIT_FAILURE;
      }
      catch (const std::exception& e)
      {
        std::cerr << "Unexpected error during parameters parsing: "
            << e.what() << std::endl;
        print_usage();
        return EXIT_FAILURE;
      }
    } // if (argc > 2)

#if defined(WIN32)
    std::wcout << L"NMEA 0183 device serial port: "
               << wide_device_name << std::endl
               << L"Read buffer size (bytes)    : "
               << read_buffer_size << std::endl
               << L"Read buffer size (messages) : "
               << message_queue_size << std::endl;
#else
    std::cout << "NMEA 0183 device serial port: "
              << device_name << std::endl
              << "Read buffer size (bytes)    : "
              << read_buffer_size << std::endl
              << "Read buffer size (messages) : "
              << message_queue_size << std::endl;
#endif // defined(WIN32)

    handler_allocator_type the_allocator;

    frame_buffer_ptr the_frame_buffer(
        boost::make_shared<frame_buffer_type>(message_queue_size));

    // An io_service for the thread pool
    // (for the executors... Java Executors API? Apache MINA :)
    boost::asio::io_service session_io_service(concurrent_count);

    session_ptr the_session(session::create(session_io_service,
        read_buffer_size, message_queue_size, "$", "\x0a"));

    // Prepare the lower layer - open the serial port
    boost::system::error_code error;
    the_session->serial_port().open(device_name, error);
    if (error)
    {
#if defined(WIN32)
      std::wstring error_message =
          ma::codecvt_cast::in(error.message(), wcodecvt);
      std::wcerr << L"Failed to open serial port: "
          << error_message << std::endl;
#else
      std::cerr << "Failed to open serial port: "
          << error.message() << std::endl;
#endif // defined(WIN32)
      return EXIT_FAILURE;
    }

    // Start session (not actually, because there are no work threads yet)
    the_session->async_start(ma::make_custom_alloc_handler(the_allocator,
        boost::bind(handle_start, the_session, boost::ref(the_allocator),
            the_frame_buffer, _1)));

    // Setup console controller
    ma::console_controller console_controller(
        boost::bind(handle_console_close, the_session));

    std::cout << "Press Ctrl+C (Ctrl+Break) to exit...\n";

    // Create work threads
    boost::thread_group work_threads;
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      work_threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &session_io_service));
    }
    work_threads.join_all();

    std::cout << "Work threads are stopped.\n";
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}

void handle_console_close(const session_ptr& session)
{
  std::cout << "User console close detected. Begin stop the session...\n";
  session->async_stop(boost::bind(handle_stop, _1));
}

void handle_start(const session_ptr& the_session,
    handler_allocator_type& the_allocator,
    const frame_buffer_ptr& frame_buffer,
    const boost::system::error_code& error)
{
  if (error)
  {
    std::cout << "Start unsuccessful. The error is: "
        << error.message() << '\n';
    return;
  }

  std::cout << "Session started successfully. Begin read...\n";

  the_session->async_read_some(frame_buffer->begin(), frame_buffer->end(),
      ma::make_custom_alloc_handler(the_allocator, boost::bind(handle_read,
          the_session, boost::ref(the_allocator), frame_buffer, _1, _2)));
}

void handle_stop(const boost::system::error_code& error)
{
  if (error)
  {
    std::cout << "The session stop was unsuccessful. The error is: "
        << error.message() << '\n';
    return;
  }

  std::cout << "The session stop was successful.\n";
}

// Only for test of cyclic_read_session::async_write_some
//void handle_write(const session_ptr& /*the_session*/,
//    const frame_ptr& /*frame*/,
//    const boost::system::error_code& /*error*/,
//    std::size_t /*bytes_transferred*/)
//{
//}

void handle_read(const session_ptr& the_session,
    handler_allocator_type& the_allocator,
    const frame_buffer_ptr& frame_buffer,
    const boost::system::error_code& error,
    std::size_t frames_transferred)
{
  print_frames(*frame_buffer, frames_transferred);

  if (boost::asio::error::eof == error)
  {
    std::cout << "Input stream was closed." \
        " But it\'s a serial port so begin read operation again...\n";

    the_session->async_read_some(frame_buffer->begin(), frame_buffer->end(),
        ma::make_custom_alloc_handler(the_allocator, boost::bind(handle_read,
            the_session, boost::ref(the_allocator), frame_buffer, _1, _2)));
    return;
  }

  if (error)
  {
    std::cout << "Read unsuccessful. Begin the session stop...\n";
    the_session->async_stop(ma::make_custom_alloc_handler(the_allocator,
        boost::bind(handle_stop, _1)));
    return;
  }

  the_session->async_read_some(frame_buffer->begin(), frame_buffer->end(),
      ma::make_custom_alloc_handler(the_allocator, boost::bind(handle_read,
          the_session, boost::ref(the_allocator), frame_buffer, _1, _2)));

  // Only for test of cyclic_read_session::async_write_some
  //frame_ptr frame = *(frame_buffer->begin());
  //the_session->async_write_some(boost::asio::buffer(*frame),
  //    boost::bind(handle_write, the_session, frame, _1, _2));
}

void print_frames(const frame_buffer_type& frames, std::size_t size)
{
  for (frame_buffer_type::const_iterator iterator = frames.begin();
      size; ++iterator, --size)
  {
    std::cout << *(*iterator) << '\n';
  }
}

void print_usage()
{
  std::cout << "Usage: nmea_client" \
      " <com_port> [<read_buffer_size> [<message_queue_size>] ]" << std::endl;
}
