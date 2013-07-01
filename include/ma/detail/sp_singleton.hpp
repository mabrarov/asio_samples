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

#include <cstddef>
#include <limits>
#include <boost/assert.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

namespace ma {
namespace detail {

class threshold : boost::noncopyable
{
private:
  typedef threshold this_type;
  typedef boost::mutex mutex_type;
  typedef boost::unique_lock<mutex_type> lock_type;
  typedef boost::condition_variable condition_variable_type;

public:
  typedef std::size_t value_type;

  explicit threshold(value_type value = 0);
  void wait();
  void inc();
  void dec();

private:
  mutex_type mutex_;
  condition_variable_type condition_variable_;
  value_type value_;
}; // class threshold

template <typename Value>
class sp_singleton : private boost::noncopyable
{
private:
  typedef sp_singleton<Value> this_type;

public:
  typedef Value value_type;
  typedef boost::shared_ptr<value_type> value_shared_ptr;

  class instance_guard;
  
  static value_shared_ptr get_nullable_instance();

  template <typename Factory>
  static value_shared_ptr get_instance(Factory);

private:
  typedef boost::weak_ptr<value_type>  value_weak_ptr;
  typedef boost::shared_ptr<threshold> threshold_ptr;

  struct static_data;

  sp_singleton();
  ~sp_singleton();

  static static_data& get_static_data(); 
}; // class sp_singleton

template <typename Value>
class sp_singleton<Value>::instance_guard
{
private:
  typedef instance_guard this_type;

public:  
  instance_guard(const this_type&);
  ~instance_guard();  

private:
  explicit instance_guard(const threshold_ptr&);
  this_type& operator=(const this_type&);

  friend class sp_singleton<Value>;

  threshold_ptr instance_threshold_;
}; // class sp_singleton<Value>::instance_guard

template <typename Value>
struct sp_singleton<Value>::static_data : private boost::noncopyable
{
  typedef boost::mutex                  mutex_type;
  typedef boost::lock_guard<mutex_type> lock_guard_type;

  static_data();
    
  mutex_type     mutex;
  value_weak_ptr weak_value;
  threshold_ptr  instance_threshold;
}; // struct sp_singleton<Value>::static_data

template <typename Value>
sp_singleton<Value>::static_data::static_data()
  : instance_threshold(boost::make_shared<threshold>())
{
}

template <typename Value>
typename sp_singleton<Value>::value_shared_ptr 
sp_singleton<Value>::get_nullable_instance()
{
  static_data& sd = get_static_data();  
  static_data::lock_guard_type lock_guard(sd.mutex);
  return sd.weak_value.lock();
}

template <typename Value>
template <typename Factory>
typename sp_singleton<Value>::value_shared_ptr 
sp_singleton<Value>::get_instance(Factory factory)
{
  static_data& sd = get_static_data();  
  static_data::lock_guard_type lock_guard(sd.mutex);
  if (value_shared_ptr shared_value = sd.weak_value.lock())
  {
    return shared_value;
  }
  sd.instance_threshold->wait();
  value_shared_ptr shared_value = 
      factory(instance_guard(sd.instance_threshold));
  sd.weak_value = shared_value;
  return shared_value;
}

template <typename Value>
typename sp_singleton<Value>::static_data&
sp_singleton<Value>::get_static_data()
{
  static static_data sd;
  return sd;
}

template <typename Value>
sp_singleton<Value>::instance_guard::instance_guard(
    const threshold_ptr& instance_threshold)
  : instance_threshold_(instance_threshold)
{
  instance_threshold_->inc();
}

template <typename Value>
sp_singleton<Value>::instance_guard::instance_guard(const this_type& other)
  : instance_threshold_(other.instance_threshold_)
{
  instance_threshold_->inc();
}

template <typename Value>
sp_singleton<Value>::instance_guard::~instance_guard()
{
  instance_threshold_->dec();
}

threshold::threshold(value_type value)
  : value_(value)
{
}

void threshold::wait()
{
  lock_type lock(mutex_);
  while (value_)
  {
    condition_variable_.wait(lock);
  }
}

void threshold::inc()
{
  lock_type lock(mutex_);
  BOOST_ASSERT_MSG(value_ != (std::numeric_limits<value_type>::max)(),
      "Value is too large. Overflow");
  ++value_;
}

void threshold::dec()
{
  lock_type lock(mutex_);
  BOOST_ASSERT_MSG(value_, "Value is too small. Underflow");
  --value_;
  if (!value_)
  {
    condition_variable_.notify_one();
  }
}
  
} // namespace detail
} // namespace ma

#endif // MA_DETAIL_SP_SINGLETON_HPP
