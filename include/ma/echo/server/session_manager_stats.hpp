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
#include <boost/cstdint.hpp>
#include <ma/limited_int.hpp>
#include <ma/echo/server/session_manager_stats_fwd.hpp>

namespace ma {
namespace echo {
namespace server {

struct session_manager_stats
{
public:
  typedef ma::limited_int<boost::uintmax_t> limited_counter;

  session_manager_stats()
    : active(0)
    , max_active(0)
    , recycled(0)
    , total_accepted()
    , active_shutdowned()
    , out_of_work()
    , timed_out()
    , error_stopped()
  {
  }

  session_manager_stats(std::size_t the_active,
      std::size_t the_max_active,
      std::size_t the_recycled,
      const limited_counter& the_total_accepted,
      const limited_counter& the_active_shutdowned,
      const limited_counter& the_out_of_work,
      const limited_counter& the_timed_out,
      const limited_counter& the_error_stopped)
    : active(the_active)
    , max_active(the_max_active)
    , recycled(the_recycled)
    , total_accepted(the_total_accepted)
    , active_shutdowned(the_active_shutdowned)
    , out_of_work(the_out_of_work)
    , timed_out(the_timed_out)
    , error_stopped(the_error_stopped)
  {    
  }

  std::size_t     active;
  std::size_t     max_active;
  std::size_t     recycled;
  limited_counter total_accepted;
  limited_counter active_shutdowned;
  limited_counter out_of_work;
  limited_counter timed_out;
  limited_counter error_stopped;
}; // struct session_manager_stats

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_STATS_HPP
