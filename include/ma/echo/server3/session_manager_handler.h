//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_SESSION_MANAGER_HANDLER_H
#define MA_ECHO_SERVER3_SESSION_MANAGER_HANDLER_H

#include <boost/system/error_code.hpp>
#include <boost/smart_ptr.hpp>
#include <ma/echo/server3/allocator.h>

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      class session_manager_start_handler
      {
      public:
        virtual void handle_start(const boost::shared_ptr<allocator>& operation_allocator,
          const boost::system::error_code& error) = 0;

      protected:
        virtual ~session_manager_start_handler()
        {
        }
      }; // class session_manager_start_handler

      class session_manager_stop_handler
      {
      public:
        virtual void handle_stop(const boost::shared_ptr<allocator>& operation_allocator,
          const boost::system::error_code& error) = 0;

      protected:
        virtual ~session_manager_stop_handler()
        {
        }
      }; // class session_manager_stop_handler

      class session_manager_wait_handler
      {
      public:
        virtual void handle_wait(const boost::shared_ptr<allocator>& operation_allocator,
          const boost::system::error_code& error) = 0;

      protected:
        virtual ~session_wait_handler()
        {
        }
      }; // class session_manager_wait_handler

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_SESSION_MANAGER_HANDLER_H
