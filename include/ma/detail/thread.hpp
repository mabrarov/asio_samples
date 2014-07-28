//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_THREAD_HPP
#define MA_DETAIL_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/barrier.hpp>

#if defined(MA_USE_CXX11_THREAD)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#else  // defined(MA_USE_CXX11_THREAD)

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/once.hpp>

#endif // defined(MA_USE_CXX11_THREAD)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_THREAD)

using std::thread;
using std::mutex;
using std::recursive_mutex;
using std::unique_lock;
using std::lock_guard;
using std::condition_variable;
using std::once_flag;
using std::call_once;
using boost::barrier;

inline std::chrono::nanoseconds to_duration(
    const boost::posix_time::time_duration& posix_duration)
{
  return std::chrono::nanoseconds(posix_duration.total_nanoseconds());
}

template<typename Lock>
bool timed_wait(std::condition_variable& condition,
    Lock& lock, const boost::posix_time::time_duration& posix_duration)
{
  return std::cv_status::timeout !=
      condition.wait_for(lock, to_duration(posix_duration));
}

inline void sleep(const boost::posix_time::time_duration& posix_duration)
{
  std::this_thread::sleep_for(to_duration(posix_duration));
}

#else  // defined(MA_USE_CXX11_THREAD)

using boost::thread;
using boost::mutex;
using boost::recursive_mutex;
using boost::unique_lock;
using boost::lock_guard;
using boost::condition_variable;
using boost::once_flag;
using boost::call_once;
using boost::barrier;

template<typename Lock>
bool timed_wait(boost::condition_variable& condition,
    Lock& lock, const boost::posix_time::time_duration& posix_duration)
{
  return condition.timed_wait(lock, posix_duration);
}

inline void sleep(const boost::posix_time::time_duration& posix_duration)
{
  boost::this_thread::sleep(posix_duration);
}

#endif // defined(MA_USE_CXX11_THREAD)

inline void count_down_and_wait(barrier& b)
{
  b.wait();
}

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_THREAD_HPP
