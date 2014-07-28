//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUPLE_HPP
#define MA_TUPLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_TUPLE)

#include <tuple>

#define MA_TUPLE      ::std::tuple
#define MA_TUPLE_GET  ::std::get
#define MA_MAKE_TUPLE ::std::make_tuple

#else  // defined(MA_USE_CXX11_STDLIB_TUPLE)

#include <boost/tuple/tuple.hpp>

#define MA_TUPLE      ::boost::tuple
#define MA_TUPLE_GET  ::boost::get
#define MA_MAKE_TUPLE ::boost::make_tuple

#endif // defined(MA_USE_CXX11_STDLIB_TUPLE)

#endif // MA_TUPLE_HPP
