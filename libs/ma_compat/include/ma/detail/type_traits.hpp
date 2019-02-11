//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
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
#include <boost/config.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/remove_cv.hpp>
#endif

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_TYPE_TRAITS)

using std::decay;

#else

#if BOOST_VERSION >= 106000

using boost::decay;

#else // BOOST_VERSION >= 106000

template <typename T>
struct decay
{
  typedef typename boost::remove_cv<typename boost::decay<T>::type>::type type;
}; // struct decay

#endif // BOOST_VERSION >= 106000

#endif // defined(MA_USE_CXX11_STDLIB_TYPE_TRAITS)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_TYPE_TRAITS_HPP
