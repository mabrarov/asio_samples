//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_STATS_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_STATS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <ma/echo/server/session_manager_stats_fwd.hpp>

namespace ma {
namespace echo {
namespace server {

struct session_manager_stats
{
public:
  session_manager_stats()
  {
    
  }

  std::size_t active_session_count;
  std::size_t max_active_session_count;
  std::size_t recycled_session_count;
  total_served_session_count;
  
}; // struct session_manager_stats

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_STATS_HPP
