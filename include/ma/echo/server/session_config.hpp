//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_CONFIG_HPP
#define MA_ECHO_SERVER_SESSION_CONFIG_HPP

#include <cstddef>
#include <boost/optional.hpp>
#include <ma/echo/server/session_config_fwd.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {
      struct session_config
      { 
        boost::optional<bool> no_delay_;
        boost::optional<int>  socket_recv_buffer_size_;
        boost::optional<int>  socket_send_buffer_size_;
        std::size_t buffer_size_;                   

        explicit session_config(std::size_t buffer_size, 
          const boost::optional<int>& socket_recv_buffer_size, 
          const boost::optional<int>& socket_send_buffer_size,
          const boost::optional<bool>& no_delay);
      }; // struct session_config
        
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_CONFIG_HPP
