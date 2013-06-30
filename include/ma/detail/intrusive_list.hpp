//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_INTRUSIVE_LIST_HPP
#define MA_DETAIL_INTRUSIVE_LIST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/addressof.hpp>

namespace ma {
namespace detail {

/// Simplified doubly linked intrusive list.
/**
 * Requirements:
 * if value is rvalue of type Value then expression
 * static_cast&lt;intrusive_list&lt;Value&gt;::base_hook&amp;&gt;(Value)
 * must be well formed and accessible from intrusive_list&lt;Value&gt;.
 */
template<typename Value>
class intrusive_list : private boost::noncopyable
{
public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  intrusive_list();

  /// Never throws
  pointer front() const;

  /// Never throws
  static pointer prev(reference value);

  /// Never throws
  static pointer next(reference value);

  /// Never throws
  void push_front(reference value);

  /// Never throws
  void erase(reference value);

  /// Never throws
  void pop_front();

  /// Never throws
  bool empty();

private:
  static base_hook& get_hook(reference value);

  pointer front_;
}; // class intrusive_list

template<typename Value>
class intrusive_list<Value>::base_hook
{
private:
  typedef base_hook this_type;

protected:
  base_hook();
  base_hook(const this_type&);
  base_hook& operator=(const this_type&);

private:
  friend class intrusive_list<Value>;
  pointer prev_;
  pointer next_;
}; // class intrusive_list::base_hook

/// Simplified singly linked intrusive list.
/**
 * Requirements:
 * if value is rvalue of type Value then expression
 * static_cast&lt;intrusive_slist&lt;Value&gt;::base_hook&amp;&gt;(Value)
 * must be well formed and accessible from intrusive_slist&lt;Value&gt;.
 */
template<typename Value>
class intrusive_slist : private boost::noncopyable
{
public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  intrusive_slist();

  /// Never throws
  pointer front() const;

  /// Never throws
  static pointer next(reference value);

  /// Never throws
  void push_front(reference value);

  /// Never throws
  void pop_front();

  /// Never throws
  bool empty();

  /// Never throws
  template<typename OtherValue>
  void push_front_reversed(intrusive_slist<OtherValue>& other);

private:
  static base_hook& get_hook(reference value);  

  pointer front_;
}; // class intrusive_slist

template<typename Value>
class intrusive_slist<Value>::base_hook
{
private:
  typedef base_hook this_type;

protected:
  base_hook();
  base_hook(const this_type&);
  base_hook& operator=(const this_type&);

private:
  friend class intrusive_slist<Value>;
  pointer next_;
}; // class intrusive_slist::base_hook

template<typename Value>
intrusive_list<Value>::base_hook::base_hook()
  : prev_(0)
  , next_(0)
{
}

template<typename Value>
intrusive_list<Value>::base_hook::base_hook(const this_type&)
  : prev_(0)
  , next_(0)
{
}

template<typename Value>
typename intrusive_list<Value>::base_hook&
intrusive_list<Value>::base_hook::operator=(const this_type&)
{
  return *this;
}

template<typename Value>
intrusive_list<Value>::intrusive_list()
  : front_(0)
{
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::front() const
{
  return front_;
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::prev(reference value)
{
  return get_hook(value).prev_;
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::next(reference value)
{
  return get_hook(value).next_;
}

template<typename Value>
void intrusive_list<Value>::push_front(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The value to push has to be unlinked");

  value_hook.next_ = front_;
  if (value_hook.next_)
  {
    base_hook& front_hook = get_hook(*value_hook.next_);
    front_hook.prev_ = boost::addressof(value);
  }
  front_ = boost::addressof(value);
}

template<typename Value>
void intrusive_list<Value>::erase(reference value)
{
  base_hook& value_hook = get_hook(value);
  if (front_ == boost::addressof(value))
  {
    front_ = value_hook.next_;
  }
  if (value_hook.prev_)
  {
    base_hook& prev_hook = get_hook(*value_hook.prev_);
    prev_hook.next_ = value_hook.next_;
  }
  if (value_hook.next_)
  {
    base_hook& next_hook = get_hook(*value_hook.next_);
    next_hook.prev_ = value_hook.prev_;
  }
  value_hook.prev_ = value_hook.next_ = 0;

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The erased value has to be unlinked");
}

template<typename Value>
void intrusive_list<Value>::pop_front()
{
  BOOST_ASSERT_MSG(front_, "The container is empty");

  base_hook& value_hook = get_hook(*front_);
  front_ = value_hook.next_;
  if (front_)
  {
    base_hook& front_hook = get_hook(*front_);
    front_hook.prev_= 0;
  }
  value_hook.next_ = value_hook.prev_ = 0;

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The popped value has to be unlinked");
}

template<typename Value>
bool intrusive_list<Value>::empty()
{
  return !front_;
}

template<typename Value>
typename intrusive_list<Value>::base_hook&
intrusive_list<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

template<typename Value>
intrusive_slist<Value>::base_hook::base_hook()
  : next_(0)
{
}

template<typename Value>
intrusive_slist<Value>::base_hook::base_hook(const this_type&)
  : next_(0)
{
}

template<typename Value>
typename intrusive_slist<Value>::base_hook&
intrusive_slist<Value>::base_hook::operator=(const this_type&)
{
  return *this;
}

template<typename Value>
intrusive_slist<Value>::intrusive_slist()
  : front_(0)
{
}

template<typename Value>
typename intrusive_slist<Value>::pointer
intrusive_slist<Value>::front() const
{
  return front_;
}

template<typename Value>
typename intrusive_slist<Value>::pointer
intrusive_slist<Value>::next(reference value)
{
  return get_hook(value).next_;
}

template<typename Value>
void intrusive_slist<Value>::push_front(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.next_, "The value to push has to be unlinked");

  value_hook.next_ = front_;
  front_ = boost::addressof(value);
}

template<typename Value>
void intrusive_slist<Value>::pop_front()
{
  BOOST_ASSERT_MSG(front_, "The container is empty");

  base_hook& value_hook = get_hook(*front_);
  front_ = value_hook.next_;  
  value_hook.next_ = 0;

  BOOST_ASSERT_MSG(!value_hook.next_, "The popped value has to be unlinked");
}

template<typename Value>
bool intrusive_slist<Value>::empty()
{
  return !front_;
}

template<typename Value>
template<typename OtherValue>
void intrusive_slist<Value>::push_front_reversed(intrusive_slist<OtherValue>& other)
{  
  typedef typename intrusive_slist<OtherValue>::pointer other_pointer;

  other_pointer value = other.front();
  while (value)
  {
    other_pointer next = other.next(*value);
    push_front(*value);
    value = next;
  }
  other.front_ = 0;
}

template<typename Value>
typename intrusive_slist<Value>::base_hook&
intrusive_slist<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_INTRUSIVE_LIST_HPP
