//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_STEADY_DEADLINE_TIMER_HPP
#define MA_STEADY_DEADLINE_TIMER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(MA_HAS_BOOST_CHRONO)
#include <boost/chrono.hpp>
#endif

namespace ma {

#if defined (MA_HAS_BOOST_CHRONO)

struct steady_time_traits
{
  typedef boost::chrono::steady_clock   steady_clock_type;
  typedef steady_clock_type::duration   duration_type;
  typedef steady_clock_type::time_point time_type;

  static bool less_than(const time_type&, const time_type&);
  static time_type add(const time_type&, const duration_type&);
  static duration_type subtract(const time_type&, const time_type&);

  static time_type now();
  static boost::posix_time::time_duration to_posix_duration(
      const duration_type&);
}; // struct steady_time_traits

typedef boost::asio::basic_deadline_timer<steady_time_traits::time_type,
    steady_time_traits> steady_deadline_timer;

#define MA_HAS_STEADY_DEADLINE_TIMER

inline steady_deadline_timer::duration_type to_steady_deadline_timer_duration(
    const boost::posix_time::time_duration& posix_duration)
{
  return boost::chrono::nanoseconds(posix_duration.total_nanoseconds());
}

inline bool steady_time_traits::less_than(
    const time_type& left, const time_type& right)
{
  return left < right;
}

inline steady_time_traits::time_type steady_time_traits::add(
    const time_type& time, const duration_type& duration)
{
  return time + duration;
}

inline steady_time_traits::duration_type steady_time_traits::subtract(
    const time_type& left, const time_type& right)
{
  return left - right;
}

inline steady_time_traits::time_type steady_time_traits::now()
{
  return steady_clock_type::now();
}

inline boost::posix_time::time_duration steady_time_traits::to_posix_duration(
    const duration_type& duration)
{
  boost::chrono::nanoseconds d(duration);
#if defined(BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
  return boost::posix_time::nanoseconds(d.count());
#else
  return boost::posix_time::microseconds(d.count() / 1000);
#endif
}

#else // defined (MA_HAS_BOOST_CHRONO)

typedef boost::asio::deadline_timer steady_deadline_timer;

#undef MA_HAS_STEADY_DEADLINE_TIMER

inline steady_deadline_timer::duration_type to_steady_deadline_timer_duration(
    const boost::posix_time::time_duration& posix_duration)
{
  return posix_duration;
}

#endif // defined (MA_HAS_BOOST_CHRONO)

} // namespace ma

#endif // MA_STEADY_DEADLINE_TIMER_HPP
