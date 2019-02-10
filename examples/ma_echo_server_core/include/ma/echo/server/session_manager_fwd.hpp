//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_FWD_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/detail/memory.hpp>

namespace ma {
namespace echo {
namespace server {

class session_manager;
typedef detail::shared_ptr<session_manager> session_manager_ptr;
typedef detail::weak_ptr<session_manager>   session_manager_weak_ptr;

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_FWD_HPP
