//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_SP_SINGLETON_HPP
#define MA_DETAIL_SP_SINGLETON_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

namespace ma {
namespace detail {

template <typename Value>
class sp_singleton : private boost::noncopyable
{
private:
  typedef sp_singleton<Value> this_type;

public:
  typedef Value value_type;
  typedef boost::shared_ptr<value_type> value_shared_ptr;

  template <typename Factory>
  static value_shared_ptr get_instance(Factory);

private:
  typedef boost::weak_ptr<value_type>   value_weak_ptr;
  typedef boost::mutex                  mutex_type;
  typedef boost::lock_guard<mutex_type> lock_guard_type;

  sp_singleton();
  ~sp_singleton();  

  static mutex_type     mutex_;
  static value_weak_ptr weak_value_;
}; // class sp_singleton

template <typename Value>
template <typename Factory>
typename sp_singleton<Value>::value_shared_ptr 
sp_singleton<Value>::get_instance(Factory factory)
{
  lock_guard_type lock_guard(mutex_);
  if (value_shared_ptr shared_value = weak_value_.lock())
  {
    return shared_value;
  }
  value_shared_ptr shared_value = factory();
  weak_value_ = shared_value;
  return shared_value;
}

template <typename Value>
typename sp_singleton<Value>::mutex_type sp_singleton<Value>::mutex_;

template <typename Value>
typename sp_singleton<Value>::value_weak_ptr sp_singleton<Value>::weak_value_;

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_SP_SINGLETON_HPP
