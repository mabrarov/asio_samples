//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_OPTIONS_HPP
#define MA_ECHO_CLIENT1_SESSION_OPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <ma/echo/client1/session_options_fwd.hpp>

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace ma {
namespace echo {
namespace client1 {

class session_options
{
public:
  typedef boost::optional<int>  optional_int;
  typedef boost::optional<bool> optional_bool;

  explicit session_options(std::size_t buffer_size,
      const optional_int& socket_recv_buffer_size = optional_int(),
      const optional_int& socket_send_buffer_size = optional_int(),
      const optional_bool& no_delay = optional_bool())
    : no_delay_(no_delay)
    , socket_recv_buffer_size_(socket_recv_buffer_size)
    , socket_send_buffer_size_(socket_send_buffer_size)
    , buffer_size_(buffer_size)
  {
    BOOST_ASSERT_MSG(buffer_size > 0, "buffer_size must be > 0");

    BOOST_ASSERT_MSG(
      !socket_recv_buffer_size || (*socket_recv_buffer_size) > 0,
      "defined socket_recv_buffer_size must be > 0");

    BOOST_ASSERT_MSG(
      !socket_send_buffer_size || (*socket_send_buffer_size) > 0,
      "defined socket_send_buffer_size must be > 0");
  }

  optional_bool no_delay() const
  {
    return no_delay_;
  }

  optional_int socket_recv_buffer_size() const
  {
    return socket_recv_buffer_size_;
  }

  optional_int socket_send_buffer_size() const
  {
    return socket_send_buffer_size_;
  }

  std::size_t buffer_size() const
  {
    return buffer_size_;
  }

private:
  optional_bool no_delay_;
  optional_int  socket_recv_buffer_size_;
  optional_int  socket_send_buffer_size_;
  std::size_t   buffer_size_;
}; // struct session_options

} // namespace client1
} // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_OPTIONS_HPP
