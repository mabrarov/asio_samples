//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_TYPE_TRAITS_HPP
#define MA_DETAIL_TYPE_TRAITS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_TYPE_TRAITS)
#include <type_traits>
#else
#include <boost/type_traits/decay.hpp>
#endif

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_TYPE_TRAITS)

using std::decay;

#else

using boost::decay;

#endif // defined(MA_USE_CXX11_STDLIB_TYPE_TRAITS)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_TYPE_TRAITS_HPP
