//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_HANDLER_H
#define MA_ECHO_CLIENT1_SESSION_HANDLER_H

#include <boost/system/error_code.hpp>
#include <boost/smart_ptr.hpp>
#include <ma/echo/client1/allocator_fwd.h>
#include <ma/echo/client1/session_handler_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {
      class session_start_handler
      {      
      private:
        typedef session_start_handler this_type;

      public:
        virtual void async_handle_start(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error) = 0;

        static void invoke(const boost::weak_ptr<this_type>& handler,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error)
        {
          if (boost::shared_ptr<this_type> this_ptr = handler.lock())
          {
            this_ptr->async_handle_start(operation_allocator, error);
          }
        }

      protected:
        virtual ~session_start_handler()
        {
        }
      }; // class session_start_handler

      class session_stop_handler
      {
      private:
        typedef session_stop_handler this_type;

      public:
        virtual void async_handle_stop(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error) = 0;

        static void invoke(const boost::weak_ptr<this_type>& handler,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error)
        {
          if (boost::shared_ptr<this_type> this_ptr = handler.lock())
          {
            this_ptr->async_handle_stop(operation_allocator, error);
          }
        }

      protected:
        virtual ~session_stop_handler()
        {
        }
      }; // class session_stop_handler

      class session_wait_handler
      {
      private:
        typedef session_wait_handler this_type;

      public:
        virtual void async_handle_wait(const allocator_ptr& operation_allocator,
          const boost::system::error_code& error) = 0;

        static void invoke(const boost::weak_ptr<this_type>& handler,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error)
        {
          if (boost::shared_ptr<this_type> this_ptr = handler.lock())
          {
            this_ptr->async_handle_wait(operation_allocator, error);
          }
        }

      protected:
        virtual ~session_wait_handler()
        {
        }
      }; // class session_wait_handler
      
    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_HANDLER_H
