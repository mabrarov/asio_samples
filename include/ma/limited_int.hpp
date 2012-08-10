//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_LIMITED_INT_HPP
#define MA_LIMITED_INT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <limits>
#include <boost/operators.hpp>

namespace ma {

template <typename Integer>
class limited_int
  : private boost::incrementable<boost::addable<
        boost::addable<limited_int<Integer>, Integer> > >
{
private:
  typedef limited_int<Integer> this_type;

public:
  typedef Integer value_type;

  limited_int()
    : overflowed_(false)
    , value_(0)
  {
  }

  limited_int(value_type value)
    : overflowed_(false)
    , value_(value)
  {
  }

  value_type value() const
  {
    return value_;
  }

  bool overflowed() const
  {
    return overflowed_;
  }

  this_type& operator+=(const this_type& other)
  {
    const value_type max = (this_type::max)();
    if (overflowed_ || other.overflowed_)
    {
      overflowed_ = true;
      value_ = max;
      return *this;
    }

    const value_type addable = max - value_;
    if (addable < other.value_)
    {
      overflowed_ = true;
      value_ = max;
      return *this;
    }

    value_ += other.value_;
    return *this;
  }

  this_type& operator+=(const value_type& other)
  {
    const value_type max = (this_type::max)();
    const value_type addable = max - value_;
    if (addable < other)
    {
      overflowed_ = true;
      value_ = max;
      return *this;
    }

    value_ += other;
    return *this;
  }

  this_type& operator++()
  {
    if (overflowed_)
    {
      return *this;
    }

    const value_type max = (this_type::max)();
    if (max == value_)
    {
      overflowed_ = true;
      value_ = max;
      return *this;
    }

    ++value_;
    return *this;
  }

  static value_type (max)()
  {
    return (std::numeric_limits<value_type>::max)();
  }

private:
  bool overflowed_;
  value_type value_;
}; // class limited_int

} // namespace ma

#endif // MA_LIMITED_INT_HPP
