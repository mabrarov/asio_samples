//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_SP_SINGLETON_HPP
#define MA_DETAIL_SP_SINGLETON_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <limits>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace detail {

template <typename Value>
class sp_singleton : private boost::noncopyable
{
private:
  typedef sp_singleton<Value> this_type;

public:
  typedef Value value_type;
  typedef shared_ptr<value_type> value_shared_ptr;

  class instance_guard;
  
  static value_shared_ptr get_nullable_instance();

  template <typename Factory>
  static value_shared_ptr get_instance(Factory);

private:
  typedef weak_ptr<value_type> value_weak_ptr;
  typedef shared_ptr<latch>    latch_ptr;

  struct static_data;
  struct static_data_factory;

  sp_singleton();
  ~sp_singleton();

  static static_data& get_static_data();

  static once_flag static_data_init_flag_;
  static static_data* static_data_;
}; // class sp_singleton

template <typename Value>
class sp_singleton<Value>::instance_guard
{
private:
  typedef instance_guard this_type;

public:  
  instance_guard(const this_type&);
  ~instance_guard();

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

private:
  explicit instance_guard(const latch_ptr&);

  friend class sp_singleton<Value>;

  latch_ptr instance_latch_;
}; // class sp_singleton<Value>::instance_guard

template <typename Value>
struct sp_singleton<Value>::static_data : private boost::noncopyable
{
  typedef mutex                  mutex_type;
  typedef lock_guard<mutex_type> lock_guard_type;

  static_data();
    
  mutex_type     value_mutex;
  value_weak_ptr weak_value;
  latch_ptr      instance_latch;
}; // struct sp_singleton<Value>::static_data

template <typename Value>
struct sp_singleton<Value>::static_data_factory
{
public:
  void operator()() const;
}; // struct sp_singleton<Value>::static_data_factory

template <typename Value>
typename sp_singleton<Value>::value_shared_ptr 
sp_singleton<Value>::get_nullable_instance()
{
  typedef typename static_data::lock_guard_type lock_guard_type;
  static_data& sd = get_static_data();  
  lock_guard_type lock_guard(sd.value_mutex);
  return sd.weak_value.lock();
}

template <typename Value>
template <typename Factory>
typename sp_singleton<Value>::value_shared_ptr 
sp_singleton<Value>::get_instance(Factory factory)
{
  typedef typename static_data::lock_guard_type lock_guard_type;
  static_data& sd = get_static_data();  
  lock_guard_type lock_guard(sd.value_mutex);
  if (value_shared_ptr shared_value = sd.weak_value.lock())
  {
    return shared_value;
  }
  sd.instance_latch->wait();
  value_shared_ptr shared_value = 
      factory(instance_guard(sd.instance_latch));
  sd.weak_value = shared_value;
  return shared_value;
}

template <typename Value>
typename sp_singleton<Value>::static_data&
sp_singleton<Value>::get_static_data()
{
  call_once(static_data_init_flag_, static_data_factory());
  BOOST_ASSERT_MSG(static_data_,
      "Singleton static data wasn't initialized correctly");
  return *static_data_;
}

#if defined(MA_USE_CXX11_STDLIB_THREAD)

template <typename Value>
once_flag sp_singleton<Value>::static_data_init_flag_;

#else  // defined(MA_USE_CXX11_STDLIB_THREAD)

template <typename Value>
once_flag sp_singleton<Value>::static_data_init_flag_ = BOOST_ONCE_INIT;

#endif // defined(MA_USE_CXX11_STDLIB_THREAD)

template <typename Value>
typename sp_singleton<Value>::static_data* 
sp_singleton<Value>::static_data_ = 0;

template <typename Value>
sp_singleton<Value>::static_data::static_data()
  : instance_latch(make_shared<latch>())
{
}

template <typename Value>
void sp_singleton<Value>::static_data_factory::operator()() const
{
  static static_data d;
  sp_singleton<Value>::static_data_ = &d;
}

template <typename Value>
sp_singleton<Value>::instance_guard::instance_guard(
    const latch_ptr& instance_latch)
  : instance_latch_(instance_latch)
{
  instance_latch_->count_up();
}

template <typename Value>
sp_singleton<Value>::instance_guard::instance_guard(const this_type& other)
  : instance_latch_(other.instance_latch_)
{
  instance_latch_->count_up();
}

template <typename Value>
sp_singleton<Value>::instance_guard::~instance_guard()
{
  instance_latch_->count_down();
}
  
} // namespace detail
} // namespace ma

#endif // MA_DETAIL_SP_SINGLETON_HPP
