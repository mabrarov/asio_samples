//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_EXECUTION_CONFIG_HPP
#define MA_ECHO_SERVER_QT_EXECUTION_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <ma/echo/server/qt/execution_config_fwd.h>

namespace ma {
namespace echo {
namespace server {
namespace qt {

struct execution_config
{
public:
  execution_config(std::size_t the_session_manager_thread_count,
      std::size_t the_session_thread_count)
    : session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)
  {
    BOOST_ASSERT_MSG(the_session_manager_thread_count > 0,
        "session_manager_thread_count must be > 0");

    BOOST_ASSERT_MSG(the_session_thread_count > 0,
        "session_thread_count must be > 0");
  }

  std::size_t session_manager_thread_count;
  std::size_t session_thread_count;
}; // struct execution_config

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_EXECUTION_CONFIG_HPP
