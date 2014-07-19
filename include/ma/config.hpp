//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONFIG_HPP
#define MA_CONFIG_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/version.hpp>
#include <boost/config.hpp>

/// Provides asio-samples configuration options.
/**
 * Listed options mostly provide aditional optimizations.
 */

//todo: add support for other compilers
//      and try to rely on standard library version instead of compiler version
#if (BOOST_VERSION >= 105500) && \
    ((defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)) \
        || (defined(BOOST_GCC) && (BOOST_GCC >= 40700)) \
        || (defined(BOOST_CLANG) && (__clang_major__ >= 3)) \
        || (defined(BOOST_INTEL) && (BOOST_INTEL_CXX_VERSION >= 1400) \
            && defined(BOOST_INTEL_STDCXX0X)))
#define MA_USE_CXX11_STDLIB_MEMORY
#define MA_USE_CXX11_STDLIB_TUPLE
#define MA_USE_CXX11_STDLIB_FUNCTIONAL
#define MA_USE_CXX11_THREAD
#elif (BOOST_VERSION >= 105500) && defined(BOOST_MSVC) && (BOOST_MSVC >= 1600)
#define MA_USE_CXX11_STDLIB_MEMORY
#define MA_USE_CXX11_STDLIB_TUPLE
#undef  MA_USE_CXX11_STDLIB_FUNCTIONAL
#undef  MA_USE_CXX11_THREAD
#else 
#undef  MA_USE_CXX11_STDLIB_MEMORY
#undef  MA_USE_CXX11_STDLIB_TUPLE
#undef  MA_USE_CXX11_STDLIB_FUNCTIONAL
#undef  MA_USE_CXX11_THREAD
#endif

#if defined(MA_USE_CXX11_STDLIB_MEMORY)

#define MA_ADDRESS_OF              ::std::addressof
#define MA_SCOPED_PTR              ::std::unique_ptr
#define MA_SHARED_PTR              ::std::shared_ptr
#define MA_WEAK_PTR                ::std::weak_ptr
#define MA_MAKE_SHARED             ::std::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::std::enable_shared_from_this
#define MA_STATIC_POINTER_CAST     ::std::static_pointer_cast

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#define MA_ADDRESS_OF              ::boost::addressof
#define MA_SCOPED_PTR              ::boost::scoped_ptr
#define MA_SHARED_PTR              ::boost::shared_ptr
#define MA_WEAK_PTR                ::boost::weak_ptr
#define MA_MAKE_SHARED             ::boost::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::boost::enable_shared_from_this
#define MA_STATIC_POINTER_CAST     ::boost::static_pointer_cast

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

#if defined(MA_USE_CXX11_STDLIB_TUPLE)

#define MA_TUPLE                   ::std::tuple
#define MA_TUPLE_GET               ::std::get
#define MA_MAKE_TUPLE              ::std::make_tuple

#else  // defined(MA_USE_CXX11_STDLIB_TUPLE)

#define MA_TUPLE                   ::boost::tuple
#define MA_TUPLE_GET               ::boost::get
#define MA_MAKE_TUPLE              ::boost::make_tuple

#endif // defined(MA_USE_CXX11_STDLIB_TUPLE)

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)

#define MA_BIND                    ::std::bind
#define MA_FUNCTION                ::std::function
#define MA_REF                     ::std::ref
#define MA_PLACEHOLDER_1           ::std::placeholders::_1
#define MA_PLACEHOLDER_2           ::std::placeholders::_2

#else  // defined(MA_USE_CXX11_STDLIB_MEMORY)

#define MA_BIND                    ::boost::bind
#define MA_FUNCTION                ::boost::function
#define MA_REF                     ::boost::ref
#define MA_PLACEHOLDER_1           _1
#define MA_PLACEHOLDER_2           _2

#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

// Check the presence of r-value references support.
#if (BOOST_VERSION >= 104000) && !defined(BOOST_NO_RVALUE_REFERENCES)
#define MA_HAS_RVALUE_REFS
#else
#undef  MA_HAS_RVALUE_REFS
#endif

#if defined(MA_HAS_RVALUE_REFS)
#define MA_RVALUE_CAST(v) (::std::move(v))
#else
#define MA_RVALUE_CAST(v) (v)
#endif

//todo: add support for other compilers (than MSVC, GCC, Clang)
// Check the level of r-value references support.
#if defined(MA_HAS_RVALUE_REFS)

/// Defines has the functors created by means of bind move constructor
/// (explicit or implicit).
/**
 * If functors created by means of bind have no move constructor then
 * some of the asio-samples explicitly define and use binders with (explicit or
 * implicit - see MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) move constructor.
 */
#if defined(BOOST_GCC) && (BOOST_GCC >= 40600)

#undef MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#undef MA_BIND_HAS_NO_MOVE_CONTRUCTOR

#elif defined(BOOST_CLANG)

#if __has_feature(cxx_implicit_moves)

#undef MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#undef MA_BIND_HAS_NO_MOVE_CONTRUCTOR

#else // __has_feature(cxx_implicit_moves)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // __has_feature(cxx_implicit_moves)

#elif (defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)) \
    || (defined(BOOST_INTEL) && (BOOST_INTEL_CXX_VERSION >= 1400) \
        && defined(BOOST_INTEL_STDCXX0X))

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)
#undef  MA_BIND_HAS_NO_MOVE_CONTRUCTOR
#else
#define MA_BIND_HAS_NO_MOVE_CONTRUCTOR
#endif

#else  // defined(BOOST_GCC) && (BOOST_GCC >= 40600)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // defined(BOOST_GCC) && (BOOST_GCC >= 40600)

#else  // defined(MA_HAS_RVALUE_REFS)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // defined(MA_HAS_RVALUE_REFS)

/// Defines does asio::io_service::strand::wrap produce "heavy" functor.
/**
 * Because of the guarantee given by Asio:
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/io_service__strand/wrap.html
 * ...
 * that, when invoked, executes code equivalent to:
 *   strand.dispatch(bind(f, a1, ... an));
 * ...
 * the result of asio::io_service::strand::wrap is too "heavy": being called by
 * means of asio_handler_invoke it does recursive-call (and double check)
 * through the related asio::io_service::strand.
 *
 * Because of Asio never calls handler directly by operator() (except the
 * default implementation of asio_handler_invoke) but always do it by the means
 * of asio_handler_invoke this guarantee is needed to Asio users only and can
 * be cut as unneeded.
 *
 * The author of Asio knows this and agrees with such a kind of optimization.
 * See asio-users mailing list history for details.
 */
#define MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER

// Check Boost.Chrono library availability
#if BOOST_VERSION >= 104700
// BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG implies Boost.Thread to be rebuilt
// i.e. custom (user) configuration of Boost C++ Libraries must be defined.
// Add #define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG to <boost/user/config.hpp>
// and rebuild Boost (or all Boost libs using Boost.DateTime).
/// Turns on usage of Boost.Chrono at ma::steady_deadline_timer implementation.
#define MA_HAS_BOOST_CHRONO
#else
/// ma::steady_deadline_timer equals to boost::asio::deadline_timer
#undef  MA_HAS_BOOST_CHRONO
#endif

// Check Boost.Timer library availability
#if BOOST_VERSION >= 104800
/// Turns on usage of Boost.Timer.
#define MA_HAS_BOOST_TIMER
#else
#undef  MA_HAS_BOOST_TIMER
#endif

// Use virtual member functions for type erasure
#undef MA_TYPE_ERASURE_NOT_USE_VIRTUAL

// Check C++11 lambdas availability
#if (BOOST_VERSION >= 104000) && !defined(BOOST_NO_LAMBDAS)
/// Turns on usage of C++11 lambdas
#define MA_HAS_LAMBDA
#else
/// Turns off usage of C++11 lambdas
#undef MA_HAS_LAMBDA
#endif

#if defined(BOOST_WINDOWS) && \
    ((defined(WINVER) && (WINVER >= 0x0500)) \
        || (defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)))
#define MA_HAS_WINDOWS_CONSOLE_SIGNAL 1
#else
#undef MA_HAS_WINDOWS_CONSOLE_SIGNAL
#endif

#if defined(BOOST_NOEXCEPT)
#define MA_NOEXCEPT BOOST_NOEXCEPT
#else
#define MA_NOEXCEPT
#endif

#endif // MA_CONFIG_HPP
