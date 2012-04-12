//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_ERROR_HPP
#define MA_NMEA_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/type_traits.hpp>

namespace ma {
namespace nmea {

/**
 * See
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-4.html
 */
const boost::system::error_category& session_error_category();

namespace session_error {

enum error_t
{
  invalid_state     = 100,
  operation_aborted = 200
}; // enum error_t

inline boost::system::error_code make_error_code(error_t e)
{
  return boost::system::error_code(
      static_cast<int>(e), session_error_category());
}

inline boost::system::error_condition make_error_condition(error_t e)
{
  return boost::system::error_condition(
      static_cast<int>(e), session_error_category());
}

} // namespace session_error

} // namespace nmea
} // namespace ma

namespace boost {
namespace system {

template <>
struct is_error_code_enum<ma::nmea::session_error::error_t>
  : public boost::true_type
{
}; // struct is_error_code_enum

} // namespace system
} // namespace boost

#endif // MA_NMEA_ERROR_HPP
