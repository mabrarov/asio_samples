//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <ma/echo/server/session_config.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {          
      session_config::session_config(std::size_t buffer_size,
        int socket_recv_buffer_size, int socket_send_buffer_size,
        bool no_delay)
        : no_delay_(no_delay)          
        , socket_recv_buffer_size_(socket_recv_buffer_size)
        , socket_send_buffer_size_(socket_send_buffer_size)
        , buffer_size_(buffer_size)         
      {
        if (1 > buffer_size)
        {
          boost::throw_exception(std::invalid_argument("too small buffer_size"));
        }
        if (0 > socket_recv_buffer_size)
        {
          boost::throw_exception(std::invalid_argument("socket_recv_buffer_size must be non negative"));
        }
        if (0 > socket_send_buffer_size)
        {
          boost::throw_exception(std::invalid_argument("socket_send_buffer_size must be non negative"));
        }
      } // session_config::session_config    
        
    } // namespace server
  } // namespace echo
} // namespace ma
