//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <csignal>
#include <ma/config.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
#include <boost/asio/detail/signal_set_service.hpp>
#endif

#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/console_close_guard.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
#include <ma/windows/console_signal.hpp>
#endif

#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {
namespace console_close_guard {

class instance_tracking_handler
{
private:
  typedef instance_tracking_handler this_type;

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

public:
  instance_tracking_handler(detail::latch& instance_counter,
      detail::latch& call_counter)
    : instance_counter_(instance_counter)
    , call_counter_(call_counter)
  {
    instance_counter_.count_up();
  }

  instance_tracking_handler(const this_type& other)
    : instance_counter_(other.instance_counter_)
    , call_counter_(other.call_counter_)
  {
    instance_counter_.count_up();
  }

#if defined(MA_HAS_RVALUE_REFS)

  instance_tracking_handler(this_type&& other)
    : instance_counter_(other.instance_counter_)
    , call_counter_(other.call_counter_)
  {
    instance_counter_.count_up();
  }

#endif

  void operator()()
  {
    call_counter_.count_up();
  }

  ~instance_tracking_handler()
  {
    instance_counter_.count_down();
  }

private:
  detail::latch& instance_counter_;
  detail::latch& call_counter_;
}; // class instance_tracking_handler

void handle_close(detail::latch& call_counter)
{
  call_counter.count_down();
}

TEST(console_close_guard, handler_call)
{
  detail::latch call_counter(1);
  ma::console_close_guard console_close_guard(detail::bind(handle_close,
      detail::ref(call_counter)));

  // Imitate SIGINT
#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
  ma::windows::console_signal::service_type& console_signal_service =
      boost::asio::use_service<ma::windows::console_signal::service_type>(
          console_close_guard.get_io_service());
  console_signal_service.deliver_signal(SIGINT);
#else
  boost::asio::detail::signal_set_service::deliver_signal(SIGINT);
#endif

  call_counter.wait();

  ASSERT_EQ(0U, call_counter.value());

  (void) console_close_guard;
} // TEST(console_close_guard, handler_call)

TEST(console_close_guard, destruction_doesnt_call_handler)
{
  detail::latch instance_counter;
  detail::latch call_counter;

  {
    ma::console_close_guard console_close_guard(instance_tracking_handler(
        instance_counter, call_counter));

    ASSERT_LE(1U, instance_counter.value());

    (void) console_close_guard;
  }

  ASSERT_EQ(0U, instance_counter.value());
  ASSERT_EQ(0U, call_counter.value());
} // TEST(console_close_guard, destruction_doesnt_call_handler)

} // namespace console_close_guard
} // namespace test
} // namespace ma
