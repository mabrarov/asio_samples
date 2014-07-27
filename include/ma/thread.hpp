//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_THREAD_HPP
#define MA_THREAD_HPP

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

#define MA_THREAD             ::std::thread
#define MA_MUTEX              ::std::mutex
#define MA_RECURSIVE_MUTEX    ::std::recursive_mutex
#define MA_UNIQUE_LOCK        ::std::unique_lock
#define MA_LOCK_GUARD         ::std::lock_guard
#define MA_CONDITION_VARIABLE ::std::condition_variable
#define MA_ONCE_FLAG          ::std::once_flag
#define MA_CALL_ONCE          ::std::call_once
#define MA_ONCE_FLAG_INIT

#else  // defined(MA_USE_CXX11_THREAD)

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#define MA_THREAD             ::boost::thread
#define MA_MUTEX              ::boost::mutex
#define MA_RECURSIVE_MUTEX    ::boost::recursive_mutex
#define MA_UNIQUE_LOCK        ::boost::unique_lock
#define MA_LOCK_GUARD         ::boost::lock_guard
#define MA_CONDITION_VARIABLE ::boost::condition_variable
#define MA_ONCE_FLAG          ::boost::once_flag
#define MA_CALL_ONCE          ::boost::call_once
#define MA_ONCE_FLAG_INIT     = BOOST_ONCE_INIT

#endif // defined(MA_USE_CXX11_THREAD)

#define MA_BARRIER            ::boost::barrier

namespace ma {

#if defined(MA_USE_CXX11_THREAD)

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

inline void count_down_and_wait(MA_BARRIER& barrier)
{
  barrier.wait();
}

} // namespace ma

#endif // MA_THREAD_HPP
