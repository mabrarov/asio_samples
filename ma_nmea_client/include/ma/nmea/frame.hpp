//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_FRAME_HPP
#define MA_NMEA_FRAME_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <ma/detail/memory.hpp>

namespace ma {
namespace nmea {

typedef std::string frame;
typedef detail::shared_ptr<frame> frame_ptr;

} // namespace nmea
} // namespace ma

#endif // MA_NMEA_FRAME_HPP
