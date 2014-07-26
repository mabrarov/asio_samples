//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_FUNCTIONAL_HPP
#define MA_FUNCTIONAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

#include <functional>

#define MA_BIND          ::std::bind
#define MA_FUNCTION      ::std::function
#define MA_REF           ::std::ref
#define MA_PLACEHOLDER_1 ::std::placeholders::_1
#define MA_PLACEHOLDER_2 ::std::placeholders::_2

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#define MA_BIND          ::boost::bind
#define MA_FUNCTION      ::boost::function
#define MA_REF           ::boost::ref
#define MA_PLACEHOLDER_1 _1
#define MA_PLACEHOLDER_2 _2

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

#endif // MA_FUNCTIONAL_HPP
