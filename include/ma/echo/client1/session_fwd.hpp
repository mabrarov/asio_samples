//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_FWD_H
#define MA_ECHO_CLIENT1_SESSION_FWD_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/shared_ptr.hpp>

namespace ma {
namespace echo {
namespace client1 {

class session;
typedef boost::shared_ptr<session> session_ptr;

} // namespace client1
} // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_FWD_H
