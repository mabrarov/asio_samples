//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_CONFIG_HPP
#define MA_ECHO_SERVER_SESSION_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ma/echo/server/session_config_fwd.hpp>

namespace ma {
namespace echo {
namespace server {

struct session_config
{
public:
  typedef boost::optional<int>             optional_int;
  typedef boost::optional<bool>            optional_bool;
  typedef boost::posix_time::time_duration time_duration;
  typedef boost::optional<time_duration>   optional_time_duration;

  explicit session_config(std::size_t the_buffer_size,
      std::size_t the_max_transfer_size,
      const optional_int& the_socket_recv_buffer_size = optional_int(),
      const optional_int& the_socket_send_buffer_size = optional_int(),
      const optional_bool& the_no_delay = optional_bool(),
      const optional_time_duration& the_inactivity_timeout =
          optional_time_duration())
    : no_delay(the_no_delay)
    , socket_recv_buffer_size(the_socket_recv_buffer_size)
    , socket_send_buffer_size(the_socket_send_buffer_size)
    , buffer_size(the_buffer_size)
    , max_transfer_size(the_max_transfer_size)
    , inactivity_timeout(the_inactivity_timeout)
  {
    BOOST_ASSERT_MSG(the_buffer_size > 0, "buffer_size must be > 0");

    BOOST_ASSERT_MSG(
      !the_socket_recv_buffer_size || (*the_socket_recv_buffer_size) >= 0,
      "defined socket_recv_buffer_size must be >= 0");

    BOOST_ASSERT_MSG(
      !the_socket_send_buffer_size || (*the_socket_send_buffer_size) >= 0,
      "defined socket_send_buffer_size must be >= 0");
  }

  optional_bool no_delay;
  optional_int  socket_recv_buffer_size;
  optional_int  socket_send_buffer_size;
  std::size_t   buffer_size;
  std::size_t   max_transfer_size;
  optional_time_duration inactivity_timeout;
}; // struct session_config

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_CONFIG_HPP
