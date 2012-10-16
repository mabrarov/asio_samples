//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_SP_INTRUSIVE_LIST_HPP
#define MA_SP_INTRUSIVE_LIST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

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
  class base_hook
  {
  private:
    typedef base_hook this_type;

  public:
    base_hook()
    {
    }

    base_hook(const this_type& other)      
    {
    }

  private:
    friend class sp_intrusive_list<value_type>;
    weak_pointer   prev_;
    shared_pointer next_;
  }; // class base_hook

  /// Never throws
  sp_intrusive_list()
    : size_(0)
  {
  }

  ~sp_intrusive_list()
  {
    clear();
  }

  /// Never throws
  const shared_pointer& front() const
  {
    return front_;
  }

  /// Never throws
  static shared_pointer prev(const shared_pointer& value)
  {
    BOOST_ASSERT_MSG(value, "The value can no not be null ptr");
    return get_hook(*value).prev_.lock();
  }

  /// Never throws
  static const shared_pointer& next(const shared_pointer& value)
  {
    BOOST_ASSERT_MSG(value, "The value can no not be null ptr");
    return get_hook(*value).next_;
  }

  /// Never throws
  void push_front(const shared_pointer& value)
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

  /// Never throws
  void erase(const shared_pointer& value)
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

  /// Never throws
  void clear()
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

  std::size_t size() const
  {
    return size_;
  }

  bool empty() const
  {
    return 0 == size_;
  }

private:
  static base_hook& get_hook(reference value)
  {
    return static_cast<base_hook&>(value);
  }

  std::size_t size_;
  shared_pointer front_;
}; // class sp_intrusive_list

} // namespace ma

#endif // MA_SP_INTRUSIVE_LIST_HPP
