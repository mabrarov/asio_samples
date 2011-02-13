//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_CONFIG_HPP
#define MA_ECHO_CLIENT1_SESSION_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/optional.hpp>
#include <ma/echo/client1/session_config_fwd.hpp>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {
      struct session_config
      { 
        boost::optional<bool> no_delay;
        boost::optional<int>  socket_recv_buffer_size;
        boost::optional<int>  socket_send_buffer_size;
        std::size_t           buffer_size;

        explicit session_config(std::size_t the_buffer_size, 
          const boost::optional<int>& the_socket_recv_buffer_size, 
          const boost::optional<int>& the_socket_send_buffer_size,
          const boost::optional<bool>& the_no_delay);
      }; // struct session_config
        
    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_CONFIG_HPP
