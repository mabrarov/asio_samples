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

#include <algorithm>
#include <utility>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/addressof.hpp>
#include <ma/config.hpp>

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
private:
  typedef intrusive_list<Value> this_type;

public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  intrusive_list();

#if defined(MA_HAS_RVALUE_REFS)

  /// Never throws
  intrusive_list(this_type&&);

  /// Never throws
  this_type& operator=(this_type&&);

#endif // defined(MA_HAS_RVALUE_REFS)

  /// Never throws
  pointer front() const;

  /// Never throws
  pointer back() const;

  /// Never throws
  static pointer prev(reference value);

  /// Never throws
  static pointer next(reference value);

  /// Never throws
  void push_front(reference value);

  /// Never throws
  void push_back(reference value);

  /// Never throws
  void erase(reference value);

  /// Never throws
  void pop_front();

  /// Never throws
  void pop_back();

  /// Never throws
  bool empty();

  /// Never throws
  void clear();

  /// Never throws
  void swap(this_type&);

  /// Never throws
  void insert_front(this_type&);

  /// Never throws
  void insert_back(this_type&);

private:
  static base_hook& get_hook(reference value);

  pointer front_;
  pointer back_;
}; // class intrusive_list

template<typename Value>
class intrusive_list<Value>::base_hook
{
private:
  typedef base_hook this_type;

protected:
  /// Never throws
  base_hook();
  /// Never throws
  base_hook(const this_type&);
  /// Never throws
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
 * static_cast&lt;intrusive_forward_list&lt;Value&gt;::base_hook&amp;&gt;(Value)
 * must be well formed and accessible from intrusive_forward_list&lt;Value&gt;.
 */
template<typename Value>
class intrusive_forward_list : private boost::noncopyable
{
private:
  typedef intrusive_forward_list<Value> this_type;

public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  intrusive_forward_list();

#if defined(MA_HAS_RVALUE_REFS)

  /// Never throws
  intrusive_forward_list(this_type&&);

  /// Never throws
  this_type& operator=(this_type&&);

#endif // defined(MA_HAS_RVALUE_REFS)

  /// Never throws
  pointer front() const;

  /// Never throws
  pointer back() const;

  /// Never throws
  static pointer next(reference value);

  /// Never throws
  void push_front(reference value);

  /// Never throws
  void push_back(reference value);

  /// Never throws
  void pop_front();

  /// Never throws
  bool empty();

  /// Never throws
  void clear();

  /// Never throws
  void swap(this_type&);

  /// Never throws
  void insert_front(this_type&);

  /// Never throws
  void insert_back(this_type&);

private:
  static base_hook& get_hook(reference value);  

  pointer front_;
  pointer back_;
}; // class intrusive_forward_list

template<typename Value>
class intrusive_forward_list<Value>::base_hook
{
private:
  typedef base_hook this_type;

protected:
  /// Never throws
  base_hook();
  /// Never throws
  base_hook(const this_type&);
  /// Never throws
  base_hook& operator=(const this_type&);

private:
  friend class intrusive_forward_list<Value>;
  pointer next_;
}; // class intrusive_forward_list::base_hook

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
  , back_(0)
{
}

#if defined(MA_HAS_RVALUE_REFS)

template<typename Value>
intrusive_list<Value>::intrusive_list(this_type&& other)
  : front_(other.front_)
  , back_(other.back_)
{
  other.front_ = other.back_ = 0;
}

template<typename Value>
typename intrusive_list<Value>::this_type& 
intrusive_list<Value>::operator=(this_type&& other)
{
  front_ = other.front_;
  back_  = other.back_;
  other.front_ = other.back_ = 0;
  return *this;
}

#endif // defined(MA_HAS_RVALUE_REFS)

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::front() const
{
  return front_;
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::back() const
{
  return back_;
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

  const pointer value_ptr = boost::addressof(value);

  value_hook.next_ = front_;
  if (value_hook.next_)
  {
    get_hook(*value_hook.next_).prev_ = value_ptr;
  }
  front_ = value_ptr;
  if (!back_)
  {
    back_ = front_;
  }

  BOOST_ASSERT_MSG(value_hook.prev_ || value_hook.next_ || (front_ == back_),
      "The pushed value has to be linked");
  BOOST_ASSERT_MSG(front_ && back_, "The list has to be not empty");
}

template<typename Value>
void intrusive_list<Value>::push_back(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The value to push has to be unlinked");

  const pointer value_ptr = boost::addressof(value);

  value_hook.prev_ = back_;
  if (value_hook.prev_)
  {
    get_hook(*value_hook.prev_).next_ = value_ptr;
  }
  back_ = value_ptr;
  if (!front_)
  {
    front_ = back_;
  }

  BOOST_ASSERT_MSG(value_hook.prev_ || value_hook.next_ || (front_ == back_),
      "The pushed value has to be linked");
  BOOST_ASSERT_MSG(front_ && back_, "The list has to be not empty");
}

template<typename Value>
void intrusive_list<Value>::erase(reference value)
{  
  base_hook& value_hook = get_hook(value);
  const pointer value_ptr = boost::addressof(value);
  if (value_ptr == front_)
  {
    front_ = value_hook.next_;
  }
  if (value_ptr == back_)
  {
    back_ = value_hook.prev_;
  }
  if (value_hook.prev_)
  {
    get_hook(*value_hook.prev_).next_ = value_hook.next_;
  }
  if (value_hook.next_)
  {
    get_hook(*value_hook.next_).prev_ = value_hook.prev_;
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
    get_hook(*front_).prev_= 0;
  }
  else
  {
    back_ = 0;
  }
  value_hook.next_ = value_hook.prev_ = 0;

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The popped value has to be unlinked");
}

template<typename Value>
void intrusive_list<Value>::pop_back()
{
  BOOST_ASSERT_MSG(back_, "The container is empty");

  base_hook& value_hook = get_hook(*back_);
  back_ = value_hook.prev_;
  if (back_)
  {
    get_hook(*back_).next_= 0;
  }
  else
  {
    front_ = 0;
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
void intrusive_list<Value>::clear()
{
  front_ = back_ = 0;
}

template<typename Value>
void intrusive_list<Value>::swap(this_type& other)
{
  std::swap(front_, other.front_);
  std::swap(back_, other.back_);
}

template<typename Value>
void intrusive_list<Value>::insert_front(this_type& other)
{
  if (other.empty())
  {
    return;
  }

  if (empty())
  {
    front_ = other.front_;
    back_  = other.back_;
    other.front_ = other.back_ = 0;
    return;
  }

  get_hook(*other.back_).next_ = front_;
  get_hook(*front_).prev_ = other.back_;  
  front_ = other.front_;
  
  other.front_ = other.back_ = 0;

  BOOST_ASSERT_MSG(other.empty(), "The moved list has to be empty");
}

template<typename Value>
void intrusive_list<Value>::insert_back(this_type& other)
{
  if (other.empty())
  {
    return;
  }

  if (empty())
  {
    front_ = other.front_;
    back_  = other.back_;
    other.front_ = other.back_ = 0;
    return;
  }

  get_hook(*back_).next_ = other.front_;
  get_hook(*other.front_).prev_ = back_;  
  back_ = other.back_;

  other.front_ = other.back_ = 0;

  BOOST_ASSERT_MSG(other.empty(), "The moved list has to be empty");
}

template<typename Value>
typename intrusive_list<Value>::base_hook&
intrusive_list<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

template<typename Value>
intrusive_forward_list<Value>::base_hook::base_hook()
  : next_(0)
{
}

template<typename Value>
intrusive_forward_list<Value>::base_hook::base_hook(const this_type&)
  : next_(0)
{
}

template<typename Value>
typename intrusive_forward_list<Value>::base_hook&
intrusive_forward_list<Value>::base_hook::operator=(const this_type&)
{
  return *this;
}

template<typename Value>
intrusive_forward_list<Value>::intrusive_forward_list()
  : front_(0)
  , back_(0)
{
}

#if defined(MA_HAS_RVALUE_REFS)

template<typename Value>
intrusive_forward_list<Value>::intrusive_forward_list(this_type&& other)
  : front_(other.front_)
  , back_(other.back_)
{
  other.front_ = other.back_ = 0;
}

template<typename Value>
typename intrusive_forward_list<Value>::this_type& 
intrusive_forward_list<Value>::operator=(this_type&& other)
{
  front_ = other.front_;
  back_  = other.back_;
  other.front_ = other.back_ = 0;
  return *this;
}

#endif // defined(MA_HAS_RVALUE_REFS)

template<typename Value>
typename intrusive_forward_list<Value>::pointer
intrusive_forward_list<Value>::front() const
{
  return front_;
}

template<typename Value>
typename intrusive_forward_list<Value>::pointer
intrusive_forward_list<Value>::back() const
{
  return back_;
}

template<typename Value>
typename intrusive_forward_list<Value>::pointer
intrusive_forward_list<Value>::next(reference value)
{
  return get_hook(value).next_;
}

template<typename Value>
void intrusive_forward_list<Value>::push_front(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.next_, "The value to push has to be unlinked");

  value_hook.next_ = front_;
  front_ = boost::addressof(value);
  if (!back_)
  {
    back_ = front_;
  }

  BOOST_ASSERT_MSG(value_hook.next_ || (front_ == back_), 
      "The pushed value has to be linked");
  BOOST_ASSERT_MSG(front_ && back_, "The list has to be not empty");
}

template<typename Value>
void intrusive_forward_list<Value>::push_back(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.next_, "The value to push has to be unlinked");

  const pointer value_ptr = boost::addressof(value);
  if (back_)
  {
    get_hook(*back_).next_ = value_ptr;
  }
  back_ = value_ptr;
  if (!front_)
  {
    front_ = back_;
  }

  BOOST_ASSERT_MSG(value_hook.next_ || (front_ == back_), 
      "The pushed value has to be linked");
  BOOST_ASSERT_MSG(front_ && back_, "The list has to be not empty");
}

template<typename Value>
void intrusive_forward_list<Value>::pop_front()
{
  BOOST_ASSERT_MSG(front_, "The container is empty");

  base_hook& value_hook = get_hook(*front_);
  front_ = value_hook.next_;
  if (!front_)
  {
    back_ = 0;
  }
  value_hook.next_ = 0;

  BOOST_ASSERT_MSG(!value_hook.next_, "The popped value has to be unlinked");
}

template<typename Value>
bool intrusive_forward_list<Value>::empty()
{
  return !front_;
}

template<typename Value>
void intrusive_forward_list<Value>::clear()
{
  front_ = back_ = 0;
}

template<typename Value>
void intrusive_forward_list<Value>::swap(this_type& other)
{
  std::swap(front_, other.front_);
  std::swap(back_, other.back_);
}

template<typename Value>
void intrusive_forward_list<Value>::insert_front(this_type& other)
{
  if (other.empty())
  {
    return;
  }

  if (empty())
  {
    front_ = other.front_;
    back_  = other.back_;
    other.front_ = other.back_ = 0;
    return;
  }

  get_hook(*other.back_).next_ = front_;
  front_ = other.front_;
  
  other.front_ = other.back_ = 0;

  BOOST_ASSERT_MSG(other.empty(), "The moved list has to be empty");
}

template<typename Value>
void intrusive_forward_list<Value>::insert_back(this_type& other)
{
  if (other.empty())
  {
    return;
  }

  if (empty())
  {
    front_ = other.front_;
    back_  = other.back_;
    other.front_ = other.back_ = 0;
    return;
  }

  get_hook(*back_).next_ = other.front_;
  back_ = other.back_;  

  other.front_ = other.back_ = 0;

  BOOST_ASSERT_MSG(other.empty(), "The moved list has to be empty");
}

template<typename Value>
typename intrusive_forward_list<Value>::base_hook&
intrusive_forward_list<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_INTRUSIVE_LIST_HPP
