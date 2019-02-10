//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_THREAD_HPP
#define MA_DETAIL_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/barrier.hpp>

#if defined(MA_USE_CXX11_STDLIB_THREAD)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#else  // defined(MA_USE_CXX11_STDLIB_THREAD)

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/once.hpp>

#endif // defined(MA_USE_CXX11_STDLIB_THREAD)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_THREAD)

using std::thread;
using std::mutex;
using std::recursive_mutex;
using std::unique_lock;
using std::lock_guard;
using std::once_flag;
using std::call_once;

class condition_variable : private boost::noncopyable
{
private:
public:  
  void notify_one();
  void notify_all();
  void wait(unique_lock<mutex>&);
  bool timed_wait(unique_lock<mutex>&, const boost::posix_time::time_duration&);

private:
  std::condition_variable condition_;
}; // class condition_variable

inline std::chrono::nanoseconds to_duration(
    const boost::posix_time::time_duration& posix_duration)
{
  return std::chrono::nanoseconds(posix_duration.total_nanoseconds());
}

namespace this_thread {

using namespace std::this_thread;

inline void sleep(const boost::posix_time::time_duration& posix_duration)
{
  std::this_thread::sleep_for(to_duration(posix_duration));
}

} // namespace this_thread

inline void condition_variable::notify_one()
{
  condition_.notify_one();
}

inline void condition_variable::notify_all()
{
  condition_.notify_all();
}

inline void condition_variable::wait(unique_lock<mutex>& lock)
{
  condition_.wait(lock);
}

inline bool condition_variable::timed_wait(unique_lock<mutex>& lock,
    const boost::posix_time::time_duration& posix_duration)
{
  return std::cv_status::timeout !=
      condition_.wait_for(lock, to_duration(posix_duration));
}

#else  // defined(MA_USE_CXX11_STDLIB_THREAD)

using boost::thread;
using boost::mutex;
using boost::recursive_mutex;
using boost::unique_lock;
using boost::lock_guard;
using boost::condition_variable;
using boost::once_flag;
using boost::call_once;
namespace this_thread = boost::this_thread;

#endif // defined(MA_USE_CXX11_STDLIB_THREAD)

class barrier : private boost::noncopyable
{
public:
  typedef unsigned int counter_type;

  explicit barrier(counter_type count);
  void count_down_and_wait();

private:
  boost::barrier barrier_;
}; // class barrier

inline barrier::barrier(counter_type count)
  : barrier_(count)
{
}

inline void barrier::count_down_and_wait()
{
  barrier_.wait();
}

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_THREAD_HPP
