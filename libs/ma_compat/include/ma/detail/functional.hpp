//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_FUNCTIONAL_HPP
#define MA_DETAIL_FUNCTIONAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

#include <functional>

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

using std::bind;
using std::function;
using std::ref;
namespace placeholders = std::placeholders;

#else  // defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

using boost::bind;
using boost::function;
using boost::ref;

namespace placeholders {

using ::_1;
using ::_2;
using ::_3;
using ::_4;
using ::_5;
using ::_6;
using ::_7;
using ::_8;
using ::_9;

} // namespace placeholders

#endif // defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_FUNCTIONAL_HPP
