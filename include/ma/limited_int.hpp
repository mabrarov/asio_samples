//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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

  limited_int();
  limited_int(value_type value);

  value_type value() const;
  bool overflowed() const;
  static value_type (max)();

  this_type& operator+=(const this_type& other);
  this_type& operator+=(const value_type& other);
  this_type& operator++();

private:
  bool overflowed_;
  value_type value_;
}; // class limited_int

template <typename Integer>
limited_int<Integer>::limited_int()
  : overflowed_(false)
  , value_(0)
{
}

template <typename Integer>
limited_int<Integer>::limited_int(value_type value)
  : overflowed_(false)
  , value_(value)
{
}

template <typename Integer>
typename limited_int<Integer>::value_type limited_int<Integer>::value() const
{
  return value_;
}

template <typename Integer>
bool limited_int<Integer>::overflowed() const
{
  return overflowed_;
}

template <typename Integer>
typename limited_int<Integer>::value_type (limited_int<Integer>::max)()
{
  return (std::numeric_limits<value_type>::max)();
}

template <typename Integer>
typename limited_int<Integer>::this_type&
limited_int<Integer>::operator+=(const this_type& other)
{
  if (overflowed_)
  {
    return *this;
  }

  const value_type max = (this_type::max)();
  if (other.overflowed_)
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

template <typename Integer>
typename limited_int<Integer>::this_type&
limited_int<Integer>::operator+=(const value_type& other)
{
  if (overflowed_)
  {
    return *this;
  }

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

template <typename Integer>
typename limited_int<Integer>::this_type& limited_int<Integer>::operator++()
{
  if (overflowed_)
  {
    return *this;
  }

  const value_type max = (this_type::max)();
  if (max == value_)
  {
    overflowed_ = true;
    return *this;
  }

  ++value_;
  return *this;
}

} // namespace ma

#endif // MA_LIMITED_INT_HPP
