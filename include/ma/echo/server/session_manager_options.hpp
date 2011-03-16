//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_OPTIONS_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_OPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <ma/echo/server/session_options.hpp>
#include <ma/echo/server/session_manager_options_fwd.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {                
      class session_manager_options
      { 
      public:
        typedef boost::asio::ip::tcp::endpoint endpoint_type;

        session_manager_options(const endpoint_type& accepting_endpoint,
          std::size_t max_session_count, 
          std::size_t recycled_session_count,
          int listen_backlog, 
          const session_options& managed_session_options)
          : listen_backlog_(listen_backlog)
          , max_session_count_(max_session_count)
          , recycled_session_count_(recycled_session_count)
          , accepting_endpoint_(accepting_endpoint)                
          , managed_session_options_(managed_session_options)
        {
          BOOST_ASSERT_MSG(max_session_count > 0, "max_session_count must be > 0");
        }

        int listen_backlog() const
        {
          return listen_backlog_;
        }

        std::size_t max_session_count() const
        {
          return max_session_count_;
        }

        std::size_t recycled_session_count() const
        {
          return recycled_session_count_;
        }

        endpoint_type accepting_endpoint() const
        {
          return accepting_endpoint_;
        }

        session_options managed_session_options() const
        {
          return managed_session_options_;
        }
        
      private:
        int            listen_backlog_;
        std::size_t    max_session_count_;
        std::size_t    recycled_session_count_;
        endpoint_type  accepting_endpoint_;
        session_options managed_session_options_;
      }; // class session_manager_options
        
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_OPTIONS_HPP
