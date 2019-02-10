//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READ_SESSION_FWD_HPP
#define MA_NMEA_CYCLIC_READ_SESSION_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/detail/memory.hpp>

namespace ma {
namespace nmea {

class cyclic_read_session;
typedef detail::shared_ptr<cyclic_read_session> cyclic_read_session_ptr;

} // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_FWD_HPP
