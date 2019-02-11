//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_UTILITY_HPP
#define MA_DETAIL_UTILITY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif

namespace ma {
namespace detail {

#if defined(MA_HAS_RVALUE_REFS)

using std::move;
using std::forward;

#else // defined(MA_HAS_RVALUE_REFS)

/// Dummy for std::move
template <typename T>
T& move(T& t)
{
  return t;
}

/// Dummy for std::move
template <typename T>
const T& move(const T& t)
{
  return t;
}

/// Dummy for std::forward
template <typename T>
T& forward(T& t)
{
  return t;
}

/// Dummy for std::forward
template <typename T>
const T& forward(const T& t)
{
  return t;
}

#endif // defined(MA_HAS_RVALUE_REFS)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_UTILITY_HPP
