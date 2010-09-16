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
          return session_io_service_;
        }

        boost::asio::io_service& session_manager_io_service()
        {
          return session_manager_io_service_;
        }

      protected:
        explicit io_service_set(boost::asio::io_service& session_io_service,
          boost::asio::io_service& session_manager_io_service)
          : session_io_service_(session_io_service)
          , session_manager_io_service_(session_manager_io_service)
        {
        }

        ~io_service_set()
        {
        }        

      private:
        boost::asio::io_service& session_io_service_;
        boost::asio::io_service& session_manager_io_service_;      
      }; // class io_service_set

      class shared_io_service_set: public io_service_set
      {
      public:
        explicit shared_io_service_set(boost::asio::io_service& shared_io_service)
          : io_service_set(shared_io_service, shared_io_service)
        {          
        }
      }; // class shared_io_service_set
      
      class seperated_io_service_set
        : private boost::base_from_member<boost::asio::io_service>
        , public io_service_set
      {
      private:
        typedef boost::base_from_member<boost::asio::io_service> io_service_member_base;

      public:
        explicit seperated_io_service_set(boost::asio::io_service& session_io_service)
          : io_service_member_base()
          , io_service_set(session_io_service, io_service_member_base::member)
        {          
        }

        explicit seperated_io_service_set(boost::asio::io_service& session_io_service, 
          std::size_t session_manager_io_service_concurrency_hint)
          : io_service_member_base(session_manager_io_service_concurrency_hint)
          , io_service_set(session_io_service, io_service_member_base::member)
        {          
        }      
      }; // class seperated_io_service_set

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_IO_SERVICE_SET_HPP
