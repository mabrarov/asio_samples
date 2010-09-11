//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config_fwd.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {                
      struct session_manager_config
      { 
        int listen_backlog_;
        std::size_t max_sessions_;
        std::size_t recycled_sessions_;
        boost::asio::ip::tcp::endpoint endpoint_;                    
        session_config session_config_;

        explicit session_manager_config(const boost::asio::ip::tcp::endpoint& endpoint,
          std::size_t max_sessions, std::size_t recycled_sessions,
          int listen_backlog, const session_config& managed_session_config);
      }; // struct session_manager_config
        
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP
