//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_MEMORY_HPP
#define MA_MEMORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <memory>

#define MA_ADDRESS_OF              ::std::addressof
#define MA_SCOPED_PTR              ::std::unique_ptr
#define MA_SHARED_PTR              ::std::shared_ptr
#define MA_WEAK_PTR                ::std::weak_ptr
#define MA_MAKE_SHARED             ::std::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::std::enable_shared_from_this
#define MA_STATIC_POINTER_CAST     ::std::static_pointer_cast

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <boost/utility/addressof.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

#define MA_ADDRESS_OF              ::boost::addressof
#define MA_SCOPED_PTR              ::boost::scoped_ptr
#define MA_SHARED_PTR              ::boost::shared_ptr
#define MA_WEAK_PTR                ::boost::weak_ptr
#define MA_MAKE_SHARED             ::boost::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::boost::enable_shared_from_this
#define MA_STATIC_POINTER_CAST     ::boost::static_pointer_cast

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

#endif // MA_MEMORY_HPP
