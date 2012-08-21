//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CONFIG_HPP
#define CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstddef>
#include <ostream>
#include <boost/assert.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>

namespace echo_server {

struct execution_config
{
public:
  typedef boost::posix_time::time_duration time_duration_type;

  execution_config(bool the_ios_per_work_thread,
      std::size_t the_session_manager_thread_count,
      std::size_t the_session_thread_count,
      const time_duration_type& the_stop_timeout)
    : ios_per_work_thread(the_ios_per_work_thread)
    , session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)
    , stop_timeout(the_stop_timeout)
  {
    BOOST_ASSERT_MSG(the_session_manager_thread_count > 0,
        "session_manager_thread_count must be > 0");

    BOOST_ASSERT_MSG(the_session_thread_count > 0,
        "session_thread_count must be > 0");
  }

  bool               ios_per_work_thread;
  std::size_t        session_manager_thread_count;
  std::size_t        session_thread_count;
  time_duration_type stop_timeout;
}; // struct execution_config

boost::program_options::options_description build_cmd_options_description(
    std::size_t hardware_concurrency);

#if defined(WIN32)

boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[]);

#else

boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[]);

#endif

void print_config(std::ostream& stream, std::size_t cpu_count,
    const execution_config& the_execution_config,
    const ma::echo::server::session_manager_config& session_manager_config);

bool is_help_mode(const boost::program_options::variables_map& options_values);

bool is_required_specified(
    const boost::program_options::variables_map& options_values);

execution_config build_execution_config(
    const boost::program_options::variables_map& options_values);

ma::echo::server::session_config build_session_config(
    const boost::program_options::variables_map& options_values);

ma::echo::server::session_manager_config build_session_manager_config(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_config& session_config);

} // namespace echo_server

#endif // CONFIG_HPP
