//
// Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_RANDOM_HPP
#define MA_DETAIL_RANDOM_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_RANDOM)

#include <random>

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <limits>
#include <boost/random.hpp>

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_RANDOM)

using std::mt19937;
using std::uniform_int_distribution;

#else  // defined(MA_USE_CXX11_STDLIB_RANDOM)

#if BOOST_VERSION >= 104700

using boost::random::mt19937;
using boost::random::uniform_int_distribution;

#else // BOOST_VERSION >= 104700

using boost::mt19937;

template <typename IntType = int>
class uniform_int_distribution : public boost::uniform_int<IntType>
{
private:
  typedef boost::uniform_int<IntType> base_type;

public:
  explicit uniform_int_distribution(IntType min_arg = 0,
      IntType max_arg = (std::numeric_limits<IntType>::max)())
    : base_type(min_arg, max_arg)
  {
  }
}; // class uniform_int_distribution

#endif // BOOST_VERSION >= 104700

#endif // defined(MA_USE_CXX11_STDLIB_RANDOM)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_RANDOM_HPP
