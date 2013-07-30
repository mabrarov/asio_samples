//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <exception>
#include <iostream>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/windows/console_signal.hpp>

namespace ma {
namespace test {
namespace windows_console_signal_destruction {

void run_test();

} // namespace windows_console_signal_destruction
} // namespace test
} // namespace ma

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    ma::test::windows_console_signal_destruction::run_test();
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}

namespace ma {
namespace test {
namespace windows_console_signal_destruction {

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

void handle_console_signal(const boost::system::error_code&)
{
  std::cout << "handle_console_signal" << std::endl;
}

typedef boost::shared_ptr<ma::windows::console_signal> console_signal_ptr;

void handle_console_signal2(const console_signal_ptr&,
    const boost::system::error_code&)
{
  std::cout << "handle_console_signal2" << std::endl;
}

typedef ma::in_place_handler_allocator<sizeof(std::size_t) * 64>
    handler_allocator;

typedef boost::shared_ptr<handler_allocator> handler_allocator_ptr;

void handle_console_signal3(const console_signal_ptr&,
    const handler_allocator_ptr&, const boost::system::error_code&)
{
  std::cout << "handle_console_signal3" << std::endl;
}

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

void run_test()
{
#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s(io_service);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s1(io_service);
    ma::windows::console_signal s2(io_service);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s1(io_service);
    ma::windows::console_signal s2(io_service);
    ma::windows::console_signal s3(io_service);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s(io_service);
    s.async_wait(&handle_console_signal);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s(io_service);
    s.async_wait(&handle_console_signal);
    s.async_wait(&handle_console_signal);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s(io_service);
    s.async_wait(&handle_console_signal);
    s.async_wait(&handle_console_signal);
    s.async_wait(&handle_console_signal);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s1(io_service);
    ma::windows::console_signal s2(io_service);
    s1.async_wait(&handle_console_signal);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s1(io_service);
    ma::windows::console_signal s2(io_service);
    s1.async_wait(&handle_console_signal);
    s1.async_wait(&handle_console_signal);
    s1.async_wait(&handle_console_signal);
  }
  {
    boost::asio::io_service io_service;
    ma::windows::console_signal s1(io_service);
    ma::windows::console_signal s2(io_service);
    s1.async_wait(&handle_console_signal);
    s2.async_wait(&handle_console_signal);
    s1.async_wait(&handle_console_signal);
    s2.async_wait(&handle_console_signal);
    s1.async_wait(&handle_console_signal);
    s2.async_wait(&handle_console_signal);
  }
  {
    handler_allocator alloc1;
    boost::asio::io_service io_service;
    {
      console_signal_ptr s1 =
          boost::make_shared<ma::windows::console_signal>(
              boost::ref(io_service));
      console_signal_ptr s2 =
          boost::make_shared<ma::windows::console_signal>(
              boost::ref(io_service));

      s1->async_wait(boost::bind(handle_console_signal2, s1, _1));
      s1->async_wait(ma::make_custom_alloc_handler(alloc1,
          boost::bind(handle_console_signal2, s1, _1)));
      s1->async_wait(boost::bind(handle_console_signal2, s1, _1));

      s2->async_wait(boost::bind(handle_console_signal2, s2, _1));
      s2->async_wait(boost::bind(handle_console_signal2, s2, _1));
      s2->async_wait(boost::bind(handle_console_signal2, s2, _1));

      s1->async_wait(boost::bind(handle_console_signal2, s2, _1));

      handler_allocator_ptr alloc2 = boost::make_shared<handler_allocator>();
      s1->async_wait(ma::make_custom_alloc_handler(*alloc2,
          boost::bind(handle_console_signal3, s1, alloc2, _1)));
    }
  }
#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
}

} // namespace windows_console_signal_destruction
} // namespace test
} // namespace ma
