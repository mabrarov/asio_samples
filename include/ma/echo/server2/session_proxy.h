//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_PROXY_H
#define MA_ECHO_SERVER2_SESSION_PROXY_H

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <ma/echo/server2/session.h>
#include <ma/echo/server2/session_completion.h>
#include <ma/echo/server2/session_proxy_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server2
    {    
      struct session_proxy : private boost::noncopyable
      {
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };

        session_proxy_weak_ptr prev_;
        session_proxy_ptr next_;
        session_ptr session_;        
        boost::asio::ip::tcp::endpoint endpoint_;        
        std::size_t pending_operations_;
        state_type state_;        
        session_completion::handler_allocator start_wait_allocator_;
        session_completion::handler_allocator stop_allocator_;

        explicit session_proxy(boost::asio::io_service& io_service,
          const session::settings& session_settings);
        ~session_proxy();
      }; // session_proxy

    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_PROXY_H