//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_LATCH_HPP
#define MA_DETAIL_LATCH_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <limits>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <ma/detail/thread.hpp>

namespace ma {
namespace detail {

class latch : private boost::noncopyable
{
private:
  typedef latch                           this_type;
  typedef detail::mutex                   mutex_type;
  typedef detail::unique_lock<mutex_type> lock_type;
  typedef detail::lock_guard<mutex_type>  lock_guard_type;
  typedef detail::condition_variable      condition_variable_type;

public:
  typedef std::size_t value_type;

  explicit latch(value_type value = 0);
  value_type value() const;  
  value_type count_up();
  value_type count_down(); 
  void wait();
  void count_down_and_wait();
  void reset(value_type value = 0);

private:
  mutable mutex_type mutex_;
  condition_variable_type condition_variable_;
  value_type value_;
}; // class latch

inline latch::latch(value_type value)
  : value_(value)
{
}

inline latch::value_type latch::value() const
{
  lock_guard_type lock_guard(mutex_);
  return value_;
}

inline latch::value_type latch::count_up()
{
  lock_guard_type lock_guard(mutex_);
  BOOST_ASSERT_MSG(value_ != (std::numeric_limits<value_type>::max)(),
      "Value is too large. Overflow");
  ++value_;
  return value_;
}

inline latch::value_type latch::count_down()
{
  lock_guard_type lock_guard(mutex_);
  BOOST_ASSERT_MSG(value_, "Value is too small. Underflow");
  --value_;
  if (!value_)
  {
    condition_variable_.notify_all();
  }
  return value_;
}

inline void latch::wait()
{
  lock_type lock(mutex_);
  while (value_)
  {
    condition_variable_.wait(lock);
  }
}

inline void latch::count_down_and_wait()
{
  if (count_down())
  {
    wait();
  }
}

inline void latch::reset(value_type value)
{
  lock_type lock(mutex_);
  value_ = value;
  if (!value_)
  {
    condition_variable_.notify_all();
  }
}
  
} // namespace detail
} // namespace ma

#endif // MA_DETAIL_LATCH_HPP
