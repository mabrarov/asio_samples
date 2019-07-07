//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/config.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#include <csignal>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/windows/console_signal.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>
#include <ma/detail/thread.hpp>

namespace ma {
namespace test {
namespace windows_console_signal {

TEST(windows_console_signal, get_io_service)
{
  boost::asio::io_service io_service;
  ma::windows::console_signal console_signal(io_service);
  boost::asio::io_service& console_signal_io_service =
      console_signal.get_io_service();
  ASSERT_EQ(detail::addressof(io_service),
      detail::addressof(console_signal_io_service));
} // TEST(windows_console_signal, get_io_service)

typedef boost::optional<boost::asio::io_service::work> optional_work;
typedef std::size_t (boost::asio::io_service::*run_io_service_func)(void);

static const run_io_service_func run_io_service = &boost::asio::io_service::run;

class io_service_thread_stop : private boost::noncopyable
{
public:
  io_service_thread_stop(optional_work& io_service_work, detail::thread& thread)
    : io_service_work_(io_service_work)
    , io_service_(ma::get_io_context(*io_service_work))
    , thread_(thread)
  {
  }

  ~io_service_thread_stop()
  {
    io_service_work_ = boost::none;
    io_service_.stop();
    if (thread_.joinable())
    {
      thread_.join();
    }
  }

private:
  optional_work& io_service_work_;
  boost::asio::io_service& io_service_;
  detail::thread& thread_;
}; // class io_service_thread_stop

void handle_ctrl_c(detail::latch& latch, const boost::system::error_code& error,
    int signal)
{
  if (!error && SIGINT == signal)
  {
    latch.count_down();
  }
}

void handle_signal(detail::latch& latch, int expected_signal,
    const boost::system::error_code& error, int signal)
{
  if (!error && expected_signal == signal)
  {
    latch.count_down();
  }
}

void handle_cancel(detail::latch& latch, const boost::system::error_code& error,
    int signal)
{
  if (boost::asio::error::operation_aborted == error && 0 == signal)
  {
    latch.count_down();
  }
}

TEST(windows_console_signal, ctrl_c_handling)
{
  detail::latch done_latch(1);

  boost::asio::io_service io_service;
  optional_work work(boost::in_place(detail::ref(io_service)));
  detail::thread thread(detail::bind(run_io_service, &io_service));
  io_service_thread_stop thread_stop(work, thread);

  ma::windows::console_signal console_signal(io_service);
  console_signal.async_wait(
      detail::bind(&handle_ctrl_c, detail::ref(done_latch),
          detail::placeholders::_1, detail::placeholders::_2));

  // Imitate Ctrl+C / Ctrl+Break
  ma::windows::console_signal::service_type& console_signal_service =
      boost::asio::use_service<ma::windows::console_signal::service_type>(
          console_signal.get_io_service());
  console_signal_service.deliver_signal(SIGINT);

  work = boost::none;
  thread.join();

  ASSERT_EQ(0U, done_latch.value());

  (void) thread_stop;
} // TEST(windows_console_signal, ctrl_c_handling)

TEST(windows_console_signal, ctrl_c_queuing)
{
  detail::latch done_latch(5);

  boost::asio::io_service io_service;
  optional_work work(boost::in_place(detail::ref(io_service)));
  detail::thread thread(detail::bind(run_io_service, &io_service));
  io_service_thread_stop thread_stop(work, thread);

  ma::windows::console_signal console_signal(io_service);

  // Enqueue / imitate Ctrl+C / Ctrl+Break
  ma::windows::console_signal::service_type& console_signal_service =
      boost::asio::use_service<ma::windows::console_signal::service_type>(
          console_signal.get_io_service());
  console_signal_service.deliver_signal(SIGINT);
  console_signal_service.deliver_signal(SIGTERM);
  console_signal_service.deliver_signal(SIGINT);
  console_signal_service.deliver_signal(SIGTERM);
  console_signal_service.deliver_signal(SIGINT);

  console_signal.async_wait(
      detail::bind(&handle_signal, detail::ref(done_latch), SIGINT,
          detail::placeholders::_1, detail::placeholders::_2));
  console_signal.async_wait(
      detail::bind(&handle_signal, detail::ref(done_latch), SIGINT,
          detail::placeholders::_1, detail::placeholders::_2));
  console_signal.async_wait(
      detail::bind(&handle_signal, detail::ref(done_latch), SIGINT,
          detail::placeholders::_1, detail::placeholders::_2));
  console_signal.async_wait(
      detail::bind(&handle_signal, detail::ref(done_latch), SIGTERM,
          detail::placeholders::_1, detail::placeholders::_2));
  console_signal.async_wait(
      detail::bind(&handle_signal, detail::ref(done_latch), SIGTERM,
          detail::placeholders::_1, detail::placeholders::_2));

  work = boost::none;
  thread.join();

  ASSERT_EQ(0U, done_latch.value());

  (void) thread_stop;
} // TEST(windows_console_signal, ctrl_c_queuing)

TEST(windows_console_signal, cancel_handling)
{
  detail::latch done_latch(1);

  boost::asio::io_service io_service;
  optional_work work(boost::in_place(detail::ref(io_service)));
  detail::thread thread(detail::bind(run_io_service, &io_service));
  io_service_thread_stop thread_stop(work, thread);

  ma::windows::console_signal console_signal(io_service);
  console_signal.async_wait(
      detail::bind(&handle_cancel, detail::ref(done_latch),
          detail::placeholders::_1, detail::placeholders::_2));

  boost::system::error_code error;
  console_signal.cancel(error);
  ASSERT_FALSE(error);

  work = boost::none;
  thread.join();

  ASSERT_EQ(0U, done_latch.value());

  (void) thread_stop;
} // TEST(windows_console_signal, cancel_handling)

void handle_console_signal(const boost::system::error_code&, int)
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
} // TEST(windows_console_signal, destruction)

} // namespace windows_console_signal
} // namespace test
} // namespace ma

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
