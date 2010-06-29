//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server3/simple_allocator.h>
#include <ma/echo/server3/session_manager.h>
#include <ma/echo/server3/session_proxy.h>

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      session_proxy::session_proxy(boost::asio::io_service& io_service,
        const boost::weak_ptr<session_manager>& session_manager,
        const session::settings& session_settings)
        : session_(boost::make_shared<session>(boost::ref(io_service), session_settings))
        , pending_operations_(0)
        , state_(ready_to_start)
        , start_wait_allocator_(boost::make_shared<simple_allocator>(256))
        , stop_allocator_(boost::make_shared<simple_allocator>(256))
        , session_manager_(session_manager)
      {
      } // session_proxy::session_proxy

      session_proxy::~session_proxy()
      {
      } // session_proxy::~session_proxy      

      void session_proxy::handle_start(const allocator_ptr& operation_allocator,
        const boost::system::error_code& error)
      {
        if (boost::shared_ptr<session_manager> session_manager = session_manager_.lock())
        {
          session_manager->strand().post
          (
            make_custom_alloc_handler
            (
              *operation_allocator, 
              boost::bind
              (
                &session_manager::handle_session_start,
                session_manager,
                shared_from_this(),
                operation_allocator,
                error
              )
            )
          );          
        }
      } // session_proxy::handle_start

      void session_proxy::handle_stop(const allocator_ptr& operation_allocator,
        const boost::system::error_code& error)
      {
        if (boost::shared_ptr<session_manager> session_manager = session_manager_.lock())
        {
          session_manager->strand().post
          (
            make_custom_alloc_handler
            (
              *operation_allocator, 
              boost::bind
              (
                &session_manager::handle_session_stop,
                session_manager,
                shared_from_this(),
                operation_allocator,
                error
              )
            )
          );
        }
      } // session_proxy::handle_stop

      void session_proxy::handle_wait(const allocator_ptr& operation_allocator,
        const boost::system::error_code& error)
      {
        if (boost::shared_ptr<session_manager> session_manager = session_manager_.lock())
        {
          session_manager->strand().post
          (
            make_custom_alloc_handler
            (
              *operation_allocator, 
              boost::bind
              (
                &session_manager::handle_session_wait,
                session_manager,
                shared_from_this(),
                operation_allocator,
                error
              )
            )
          );
        }
      } // session_proxy::handle_wait

    } // namespace server3
  } // namespace echo
} // namespace ma