//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <ma/echo/server/session_manager_config.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {          
      session_manager_config::session_manager_config(const boost::asio::ip::tcp::endpoint& endpoint,
        std::size_t max_sessions, std::size_t recycled_sessions,
        int listen_backlog, const session_config& managed_session_config)
        : listen_backlog_(listen_backlog)
        , max_sessions_(max_sessions)
        , recycled_sessions_(recycled_sessions)
        , endpoint_(endpoint)                
        , session_config_(managed_session_config)
      {
        if (1 > max_sessions_)
        {
          boost::throw_exception(std::invalid_argument("maximum sessions number must be >= 1"));
        }
      } // session_manager_config::session_manager_config
              
    } // namespace server
  } // namespace echo
} // namespace ma

