//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/config.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#include <windows.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/windows/console_signal.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {

//todo: Change smth in CTest parameters - current implementation breaks CTest.
//      CTest handles Ctrl+C instead of the test application
//namespace windows_console_signal_ctrl_c_handling {
//
//void handle_ctrl_c(detail::latch& latch)
//{
//  latch.count_down();
//}
//
//TEST(windows_console_signal, ctrl_c_handling)
//{
//  detail::latch done_latch(1);
//  boost::asio::io_service io_service;
//  ma::windows::console_signal console_signal(io_service);
//  console_signal.async_wait(
//      detail::bind(&handle_ctrl_c, detail::ref(done_latch)));
//  BOOL success = ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
//  ASSERT_NE(0, success);
//  io_service.run();
//  ASSERT_EQ(0U, done_latch.value());
//}
//
//} // namespace windows_console_signal_ctrl_c_handling

//todo: Change smth in CTest parameters - current implementation breaks CTest.
//      CTest handles Ctrl+Break instead of the test application
//namespace windows_console_signal_ctrl_break_handling {
//
//void handle_ctrl_c(detail::latch& latch)
//{
//  latch.count_down();
//}
//
//TEST(windows_console_signal, ctrl_break_handling)
//{
//  detail::latch done_latch(1);
//  boost::asio::io_service io_service;
//  ma::windows::console_signal console_signal(io_service);
//  console_signal.async_wait(
//      detail::bind(&handle_ctrl_c, detail::ref(done_latch)));
//  BOOL success = ::GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
//  ASSERT_NE(0, success);
//  io_service.run();
//  ASSERT_EQ(0U, done_latch.value());
//}
//
//} // namespace windows_console_signal_ctrl_break_handling

namespace windows_console_signal_destruction {

void handle_console_signal(const boost::system::error_code&)
{
  std::cout << "handle_console_signal" << std::endl;
}

typedef detail::shared_ptr<ma::windows::console_signal> console_signal_ptr;

void handle_console_signal2(const console_signal_ptr&,
    const boost::system::error_code&)
{
  std::cout << "handle_console_signal2" << std::endl;
}

typedef ma::in_place_handler_allocator<sizeof(std::size_t) * 64>
    handler_allocator;

typedef detail::shared_ptr<handler_allocator> handler_allocator_ptr;

void handle_console_signal3(const console_signal_ptr&,
    const handler_allocator_ptr&, const boost::system::error_code&)
{
  std::cout << "handle_console_signal3" << std::endl;
}

TEST(windows_console_signal, destruction)
{
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
      console_signal_ptr s1 = detail::make_shared<ma::windows::console_signal>(
          detail::ref(io_service));
      console_signal_ptr s2 = detail::make_shared<ma::windows::console_signal>(
          detail::ref(io_service));

      s1->async_wait(
          detail::bind(handle_console_signal2, s1, detail::placeholders::_1));
      s1->async_wait(ma::make_custom_alloc_handler(alloc1,
          detail::bind(handle_console_signal2, s1, detail::placeholders::_1)));
      s1->async_wait(
          detail::bind(handle_console_signal2, s1, detail::placeholders::_1));

      s2->async_wait(
          detail::bind(handle_console_signal2, s2, detail::placeholders::_1));
      s2->async_wait(
          detail::bind(handle_console_signal2, s2, detail::placeholders::_1));
      s2->async_wait(
          detail::bind(handle_console_signal2, s2, detail::placeholders::_1));

      s1->async_wait(
          detail::bind(handle_console_signal2, s2, detail::placeholders::_1));

      {
        handler_allocator_ptr alloc2 = detail::make_shared<handler_allocator>();
        s1->async_wait(ma::make_custom_alloc_handler(*alloc2,
            detail::bind(handle_console_signal3, s1, alloc2, 
                detail::placeholders::_1)));
      }
    }
  }
} // TEST(windows_console_signal, simple)

} // namespace windows_console_signal_destruction
} // namespace test
} // namespace ma

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
