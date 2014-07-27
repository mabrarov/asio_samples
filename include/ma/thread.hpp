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

#if defined(MA_USE_CXX11_THREAD)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#define MA_THREAD             ::std::thread
#define MA_MUTEX              ::std::mutex
#define MA_UNIQUE_LOCK        ::std::unique_lock
#define MA_LOCK_GUARD         ::std::lock_guard
#define MA_CONDITION_VARIABLE ::std::condition_variable

#else  // defined(MA_USE_CXX11_THREAD)

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#define MA_THREAD             ::boost::thread
#define MA_MUTEX              ::boost::mutex
#define MA_UNIQUE_LOCK        ::boost::unique_lock
#define MA_LOCK_GUARD         ::boost::lock_guard
#define MA_CONDITION_VARIABLE ::boost::condition_variable

#endif // defined(MA_USE_CXX11_THREAD)

namespace ma {

#if defined(MA_USE_CXX11_THREAD)

template<typename Lock>
bool timed_wait(std::condition_variable& condition,
    Lock& lock, const boost::posix_time::time_duration& posix_duration)
{
  return std::cv_status::timeout != condition.wait_for(lock, 
      std::chrono::nanoseconds(posix_duration.total_nanoseconds()));
}

#else  // defined(MA_USE_CXX11_THREAD)

template<typename Lock>
bool timed_wait(boost::condition_variable& condition,
    Lock& lock, const boost::posix_time::time_duration& posix_duration)
{
  return condition.timed_wait(lock, posix_duration);
}

#endif // defined(MA_USE_CXX11_THREAD)

} // namespace ma

#endif // MA_THREAD_HPP
