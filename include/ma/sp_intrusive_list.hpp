//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_SP_INTRUSIVE_LIST_HPP
#define MA_SP_INTRUSIVE_LIST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>

namespace ma {

/// Simplified double-linked intrusive list of boost::shared_ptr.
/**
 * Const time insertion of boost::shared_ptr.
 * Const time deletion of boost::shared_ptr (deletion by value).
 *
 * Requirements:
 * if value is rvalue of type Value then expression
 * static_cast&lt;sp_intrusive_list&lt;Value&gt;::base_hook&amp;&gt;(value)
 * must be well formed and accessible from sp_intrusive_list&lt;Value&gt;.
 */
template <typename Value>
class sp_intrusive_list : private boost::noncopyable
{
public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;
  typedef boost::weak_ptr<Value>   weak_pointer;
  typedef boost::shared_ptr<Value> shared_pointer;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  sp_intrusive_list();
  ~sp_intrusive_list();

  /// Never throws
  const shared_pointer& front() const;

  /// Never throws
  static shared_pointer prev(const shared_pointer& value);

  /// Never throws
  static const shared_pointer& next(const shared_pointer& value);

  /// Never throws
  void push_front(const shared_pointer& value);

  /// Never throws
  void erase(const shared_pointer& value);

  /// Never throws
  void clear();

  std::size_t size() const;

  bool empty() const;

private:
  static base_hook& get_hook(reference value);

  std::size_t    size_;
  shared_pointer front_;
}; // class sp_intrusive_list

template <typename Value>
class sp_intrusive_list<Value>::base_hook
{
private:
  typedef base_hook this_type;

public:
  base_hook();
  base_hook(const this_type&);
  base_hook& operator=(const this_type&);

private:
  friend class sp_intrusive_list<Value>;
  weak_pointer   prev_;
  shared_pointer next_;
}; // class sp_intrusive_list::base_hook

template <typename Value>
sp_intrusive_list<Value>::base_hook::base_hook()
{
}

template <typename Value>
sp_intrusive_list<Value>::base_hook::base_hook(const this_type&)
{
}

template <typename Value>
typename sp_intrusive_list<Value>::base_hook&
sp_intrusive_list<Value>::base_hook::operator=(const this_type&)
{
  return *this;
}

template <typename Value>
sp_intrusive_list<Value>::sp_intrusive_list()
  : size_(0)
{
}

template <typename Value>
sp_intrusive_list<Value>::~sp_intrusive_list()
{
  clear();
}

template <typename Value>
const typename sp_intrusive_list<Value>::shared_pointer&
sp_intrusive_list<Value>::front() const
{
  return front_;
}

template <typename Value>
typename sp_intrusive_list<Value>::shared_pointer
sp_intrusive_list<Value>::prev(const shared_pointer& value)
{
  BOOST_ASSERT_MSG(value, "The value can no not be null ptr");
  return get_hook(*value).prev_.lock();
}

template <typename Value>
const typename sp_intrusive_list<Value>::shared_pointer&
sp_intrusive_list<Value>::next(const shared_pointer& value)
{
  BOOST_ASSERT_MSG(value, "The value can no not be null ptr");
  return get_hook(*value).next_;
}

template <typename Value>
void sp_intrusive_list<Value>::push_front(const shared_pointer& value)
{
  BOOST_ASSERT_MSG(value, "The value can no not be null ptr");

  base_hook& value_hook = get_hook(*value);

  BOOST_ASSERT_MSG(!value_hook.prev_.lock() && !value_hook.next_,
      "The value to push has to be not linked");

  value_hook.next_ = front_;
  if (front_)
  {
    base_hook& front_hook = get_hook(*front_);
    front_hook.prev_ = value;
  }
  front_ = value;
  ++size_;
}

template <typename Value>
void sp_intrusive_list<Value>::erase(const shared_pointer& value)
{
  BOOST_ASSERT_MSG(value, "The value can no not be null ptr");

  base_hook& value_hook = get_hook(*value);
  if (value == front_)
  {
    front_ = value_hook.next_;
  }
  const shared_pointer prev = value_hook.prev_.lock();
  if (prev)
  {
    base_hook& prev_hook = get_hook(*prev);
    prev_hook.next_ = value_hook.next_;
  }
  if (value_hook.next_)
  {
    base_hook& next_hook = get_hook(*value_hook.next_);
    next_hook.prev_ = value_hook.prev_;
  }
  value_hook.prev_.reset();
  value_hook.next_.reset();
  --size_;

  BOOST_ASSERT_MSG(!value_hook.prev_.lock() && !value_hook.next_,
      "The erased value has to be unlinked");
}

template <typename Value>
void sp_intrusive_list<Value>::clear()
{
  // We don't want to have recusrive calls of wrapped_session's destructor
  // because the deep of such recursion may be equal to the size of list.
  // The last can be too great for the stack.
  while (front_)
  {
    base_hook& front_hook = get_hook(*front_);
    shared_pointer tmp;
    tmp.swap(front_hook.next_);
    front_hook.prev_.reset();
    front_.swap(tmp);
  }
  size_ = 0;
}

template <typename Value>
std::size_t sp_intrusive_list<Value>::size() const
{
  return size_;
}

template <typename Value>
bool sp_intrusive_list<Value>::empty() const
{
  return 0 == size_;
}

template <typename Value>
typename sp_intrusive_list<Value>::base_hook&
sp_intrusive_list<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

} // namespace ma

#endif // MA_SP_INTRUSIVE_LIST_HPP
