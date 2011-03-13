//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config_fwd.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {                
      class session_manager_config
      { 
      public:
        typedef boost::asio::ip::tcp::endpoint endpoint_type;

        session_manager_config(
          const endpoint_type& accepting_endpoint,
          std::size_t max_session_count, 
          std::size_t recycled_session_count,
          int listen_backlog, 
          const session_config& managed_session_config)
          : listen_backlog_(listen_backlog)
          , max_session_count_(max_session_count)
          , recycled_session_count_(recycled_session_count)
          , accepting_endpoint_(accepting_endpoint)                
          , managed_session_config_(managed_session_config)
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

        session_config managed_session_config() const
        {
          return managed_session_config_;
        }
        
      private:
        int            listen_backlog_;
        std::size_t    max_session_count_;
        std::size_t    recycled_session_count_;
        endpoint_type  accepting_endpoint_;
        session_config managed_session_config_;
      }; // class session_manager_config
        
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP
