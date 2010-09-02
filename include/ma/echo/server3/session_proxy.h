//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_SESSION_PROXY_H
#define MA_ECHO_SERVER3_SESSION_PROXY_H

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <ma/echo/server3/allocator_fwd.h>
#include <ma/echo/server3/session.h>
#include <ma/echo/server3/session_manager_fwd.h>
#include <ma/echo/server3/session_handler.h>
#include <ma/echo/server3/session_proxy_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server3
    {    
      class session_proxy 
        : private boost::noncopyable
        , public session_start_handler
        , public session_stop_handler
        , public session_wait_handler
        , public boost::enable_shared_from_this<session_proxy>
      {
      public:
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };

        state_type state_;
        std::size_t pending_operations_;
        session_proxy_weak_ptr prev_;
        session_proxy_ptr next_;        
        session_ptr session_;        
        boost::asio::ip::tcp::endpoint endpoint_;                        
        allocator_ptr start_wait_allocator_;
        allocator_ptr stop_allocator_;
        boost::weak_ptr<session_manager> session_manager_;
        
        explicit session_proxy(boost::asio::io_service& io_service,
          const boost::weak_ptr<session_manager>& session_manager,
          const session::settings& session_settings);
        ~session_proxy();
        
        void async_handle_start(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);
        void async_handle_stop(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);
        void async_handle_wait(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);
      }; // session_proxy

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_SESSION_PROXY_H