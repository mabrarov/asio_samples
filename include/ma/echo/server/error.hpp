//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_ERROR_HPP
#define MA_ECHO_SERVER_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/type_traits.hpp>

namespace ma {
namespace echo {
namespace server {

/**
 * See
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-4.html
 */
const boost::system::error_category& server_error_category();

namespace server_error {

enum error_t
{
  invalid_state      = 100,
  operation_aborted  = 200,
  inactivity_timeout = 300,
  no_memory          = 400,
  out_of_work        = 500
}; // enum error_t

inline boost::system::error_code make_error_code(error_t e)
{
  return boost::system::error_code(static_cast<int>(e),
      server_error_category());
}

inline boost::system::error_condition make_error_condition(error_t e)
{
  return boost::system::error_condition(static_cast<int>(e),
      server_error_category());
}

} // namespace server_error

} // namespace server
} // namespace echo
} // namespace ma

namespace boost {
namespace system {

template <>
struct is_error_code_enum<ma::echo::server::server_error::error_t>
  : public boost::true_type
{
}; // struct is_error_code_enum

} // namespace system
} // namespace boost

#endif // MA_ECHO_SERVER_ERROR_HPP
