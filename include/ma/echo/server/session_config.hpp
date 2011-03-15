//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/optional.hpp>
#include <ma/echo/server/session_config_fwd.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {
      class session_config
      { 
      public:        
        typedef boost::optional<int>  optional_int;
        typedef boost::optional<bool> optional_bool;

        explicit session_config(std::size_t buffer_size, 
          const optional_int& socket_recv_buffer_size = optional_int(), 
          const optional_int& socket_send_buffer_size = optional_int(),
          const optional_bool& no_delay = optional_bool())
          : no_delay_(no_delay)
          , socket_recv_buffer_size_(socket_recv_buffer_size)
          , socket_send_buffer_size_(socket_send_buffer_size)
          , buffer_size_(buffer_size)         
        {
          BOOST_ASSERT_MSG(buffer_size > 0, "buffer_size must be > 0");

          BOOST_ASSERT_MSG(!socket_recv_buffer_size || (*socket_recv_buffer_size) > 0,
            "defined socket_recv_buffer_size must be > 0");

          BOOST_ASSERT_MSG(!socket_send_buffer_size || (*socket_send_buffer_size) > 0,
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
      }; // struct session_config
        
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_CONFIG_HPP
