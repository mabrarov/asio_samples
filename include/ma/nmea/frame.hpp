//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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
#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

namespace ma {
namespace nmea {

typedef std::string frame;
typedef MA_SHARED_PTR<frame> frame_ptr;

} // namespace nmea
} // namespace ma

#endif // MA_NMEA_FRAME_HPP
