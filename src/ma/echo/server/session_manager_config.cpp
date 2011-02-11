//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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
      session_manager_config::session_manager_config(const boost::asio::ip::tcp::endpoint& the_accepting_endpoint,
        std::size_t the_max_session_count, std::size_t the_recycled_session_count,
        int the_listen_backlog, const session_config& the_managed_session_config)
        : listen_backlog(the_listen_backlog)
        , max_session_count(the_max_session_count)
        , recycled_session_count(the_recycled_session_count)
        , accepting_endpoint(the_accepting_endpoint)                
        , managed_session_config(the_managed_session_config)
      {
        if (1 > the_max_session_count)
        {
          boost::throw_exception(std::invalid_argument("maximum sessions number must be >= 1"));
        }
      } // session_manager_config::session_manager_config
              
    } // namespace server
  } // namespace echo
} // namespace ma

