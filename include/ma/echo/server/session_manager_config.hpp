//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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

namespace ma {
namespace echo {
namespace server {

struct session_manager_config
{
public:
  typedef boost::asio::ip::tcp::endpoint endpoint_type;

  session_manager_config(
      const endpoint_type& accepting_endpoint,
      std::size_t max_session_count,
      std::size_t recycled_session_count,
      std::size_t max_stopping_sessions,
      int listen_backlog,
      const session_config& managed_session_config);

  int            listen_backlog;
  std::size_t    max_session_count;
  std::size_t    recycled_session_count;
  std::size_t    max_stopping_sessions;
  endpoint_type  accepting_endpoint;
  session_config managed_session_config;
}; // struct session_manager_config

inline session_manager_config::session_manager_config(
    const endpoint_type& the_accepting_endpoint,
    std::size_t the_max_session_count,
    std::size_t the_recycled_session_count,
    std::size_t the_max_stopping_sessions,
    int the_listen_backlog,
    const session_config& the_managed_session_config)
  : listen_backlog(the_listen_backlog)
  , max_session_count(the_max_session_count)
  , recycled_session_count(the_recycled_session_count)
  , max_stopping_sessions(the_max_stopping_sessions)
  , accepting_endpoint(the_accepting_endpoint)
  , managed_session_config(the_managed_session_config)
{
  BOOST_ASSERT_MSG(the_max_session_count > 0,
      "max_session_count must be > 0");
}

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_CONFIG_HPP
