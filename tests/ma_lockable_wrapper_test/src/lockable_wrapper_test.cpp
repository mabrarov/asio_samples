//
// Copyright (c) 2010-2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <ma/lockable_wrapped_handler.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/utility.hpp>
#include <ma/test/io_service_pool.hpp>

namespace ma {
namespace test {

void count_down(detail::latch& latch)
{
  latch.count_down();
}

namespace lockable_wrapper {

typedef std::string data_type;
typedef detail::function<void(void)> continuation;

void mutating_func1(data_type& d, const continuation& cont)
{
  d += " 1 ";
  cont();
}

TEST(lockable_wrapper, simple)
{
  typedef detail::mutex                  mutex_type;
  typedef detail::lock_guard<mutex_type> lock_guard_type;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);


  mutex_type mutex;
  std::string data;
  {
    lock_guard_type lock_guard(mutex);
    data = "0";
  }

  detail::latch done_latch(1);
  {
    lock_guard_type data_guard(mutex);

    io_service.post(ma::make_lockable_wrapped_handler(mutex, detail::bind(
        mutating_func1, detail::ref(data), continuation(
            detail::bind(count_down, detail::ref(done_latch))))));

    data = "Zero";
  }
  done_latch.wait();

  {
    lock_guard_type lock_guard(mutex);
    ASSERT_EQ("Zero 1 ", data);
  }
} // TEST(lockable_wrapper, simple)

} // namespace lockable_wrapper
} // namespace test
} // namespace ma
