//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_IO_SERVICE_SET_HPP
#define MA_ECHO_SERVER_IO_SERVICE_SET_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <ma/echo/server/io_service_set_fwd.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      class io_service_set : private boost::noncopyable
      {
      public:
        boost::asio::io_service& session_io_service()
        {
          return *session_io_service_;
        }

        boost::asio::io_service& session_manager_io_service()
        {
          return *session_manager_io_service_;
        }

      protected:
        explicit io_service_set()
          : session_io_service_(0)
          , session_manager_io_service_(0)
        {
        }

        ~io_service_set()
        {
        }

        void set_session_io_service(boost::asio::io_service& io_service)
        {
          session_io_service_ = &io_service;
        }

        void set_session_manager_io_service(boost::asio::io_service& io_service)
        {
          session_manager_io_service_ = &io_service;
        }

      private:
        boost::asio::io_service* session_io_service_;
        boost::asio::io_service* session_manager_io_service_;      
      }; // class io_service_set

      class shared_io_service_set: public io_service_set
      {
      public:
        explicit shared_io_service_set(boost::asio::io_service& shared_io_service)          
        {
          set_session_io_service(shared_io_service);
          set_session_manager_io_service(shared_io_service);
        }
      }; // class shared_io_service_set
      
      class seperated_io_service_set: public io_service_set
      {
      public:
        explicit seperated_io_service_set(boost::asio::io_service& session_io_service)
          : session_manager_io_service_()
        {
          set_session_io_service(session_io_service);
          set_session_manager_io_service(session_manager_io_service_);          
        }

        explicit seperated_io_service_set(boost::asio::io_service& session_io_service, 
          std::size_t session_manager_io_service_concurrency_hint)
          : session_manager_io_service_(session_manager_io_service_concurrency_hint)
        {
          set_session_io_service(session_io_service);
          set_session_manager_io_service(session_manager_io_service_);          
        }

      private:
        boost::asio::io_service session_manager_io_service_;
      }; // class seperated_session_manager_service_set

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_IO_SERVICE_SET_HPP
