//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_FWD_HPP
#define MA_ECHO_SERVER_SESSION_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STD)
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#endif // defined(MA_USE_CXX11_STD)

namespace ma {
namespace echo {
namespace server {

class session;
typedef MA_SHARED_PTR<session> session_ptr;

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_FWD_HPP
