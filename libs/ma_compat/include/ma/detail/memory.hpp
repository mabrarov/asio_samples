//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_MEMORY_HPP
#define MA_DETAIL_MEMORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <memory>

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <boost/utility/addressof.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

namespace ma {
namespace detail {

#if defined(MA_USE_CXX11_STDLIB_MEMORY)

using std::addressof;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;
using std::enable_shared_from_this;
using std::static_pointer_cast;

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

using boost::addressof;
using boost::scoped_ptr;
using boost::scoped_array;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::make_shared;
using boost::enable_shared_from_this;
using boost::static_pointer_cast;

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_MEMORY_HPP
