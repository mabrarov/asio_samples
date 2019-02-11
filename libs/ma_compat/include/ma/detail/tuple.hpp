//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_TUPLE_HPP
#define MA_DETAIL_TUPLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_TUPLE)

#include <tuple>

#else  // defined(MA_USE_CXX11_STDLIB_TUPLE)

#include <boost/tuple/tuple.hpp>

#endif // defined(MA_USE_CXX11_STDLIB_TUPLE)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_TUPLE)

using std::tuple;
using std::get;
using std::make_tuple;

#else  // defined(MA_USE_CXX11_STDLIB_TUPLE)

using boost::tuple;
using boost::get;
using boost::make_tuple;

#endif // defined(MA_USE_CXX11_STDLIB_TUPLE)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_TUPLE_HPP
