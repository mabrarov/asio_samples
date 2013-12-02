//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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

// Check the level of r-value references support.
#if (BOOST_VERSION >= 104000) && !defined(BOOST_NO_RVALUE_REFERENCES)

/// Turns on move semantic support.
#define MA_HAS_RVALUE_REFS

/// Defines has the functors created by means of boost::bind move constructor
/// (explicit or implicit).
/**
 * If functors created by means of boost::bind have no move constructor then
 * some of the asio-samples explicitly define and use binders with (explicit or
 * implicit - see MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) move constructor.
 */
#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)

/// Turns off explicit definition of move constructor (and copy constructor).
#undef MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
/// Turns off usage of home-grown binders with move semantic support.
#undef MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#elif defined(__clang__)

#if __has_feature(cxx_implicit_moves)

/// Turns off explicit definition of move constructor (and copy constructor).
#undef MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
/// Turns off usage of home-grown binders with move semantic support.
#undef MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#else // __has_feature(cxx_implicit_moves)

/// Turns on explicit definition of move constructor (and copy constructor).
#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
/// Turns on usage of home-grown binders with move semantic support.
#define MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // __has_feature(cxx_implicit_moves)

#else

/// Turns on explicit definition of move constructor (and copy constructor).
#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
/// Turns on usage of home-grown binders with move semantic support.
#define MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)

#else  // (BOOST_VERSION >= 104000) && !defined(BOOST_NO_RVALUE_REFERENCES)

#undef  MA_HAS_RVALUE_REFS
#undef  MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // (BOOST_VERSION >= 104000) && !defined(BOOST_NO_RVALUE_REFERENCES)

#if defined(MA_HAS_RVALUE_REFS)

#define MA_RVALUE_CAST(v) (::std::move(v))

#else

#define MA_RVALUE_CAST(v) (v)

#endif // defined(MA_HAS_RVALUE_REFS)

/// Defines does asio::io_service::strand::wrap produce "heavy" functor.
/**
 * Because of the guarantee given by Asio:
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/io_service__strand/wrap.html
 * ...
 * that, when invoked, executes code equivalent to:
 *   strand.dispatch(boost::bind(f, a1, ... an));
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

#else  // BOOST_VERSION >= 104700

/// ma::steady_deadline_timer equals to boost::asio::deadline_timer
#undef MA_HAS_BOOST_CHRONO

#endif // BOOST_VERSION >= 104700

// Check Boost.Timer library availability
#if BOOST_VERSION >= 104800

/// Turns on usage of Boost.Timer.
#define MA_HAS_BOOST_TIMER

#else  // BOOST_VERSION >= 104800

#undef MA_HAS_BOOST_TIMER

#endif // BOOST_VERSION >= 104800

// Use vurtual member functions for type erasure
#define MA_TYPE_ERASURE_USE_VURTUAL

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

//todo: add other compiler support
#if (BOOST_VERSION >= 105500) && \
    ((defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)) \
        || (defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 7)))
#define MA_USE_CXX11_STDLIB
#else
#undef  MA_USE_CXX11_STDLIB
#endif

#if defined(MA_USE_CXX11_STDLIB)

#define MA_SHARED_PTR              ::std::shared_ptr
#define MA_WEAK_PTR                ::std::weak_ptr
#define MA_MAKE_SHARED             ::std::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::std::enable_shared_from_this
#define MA_BIND                    ::std::bind
#define MA_PLACEHOLDER_1           ::std::placeholders::_1
#define MA_PLACEHOLDER_2           ::std::placeholders::_2
#define MA_REF                     ::std::ref
#define MA_FUNCTION                ::std::function
#define MA_ADDRESS_OF              ::std::addressof
#define MA_STATIC_POINTER_CAST     ::std::static_pointer_cast
#define MA_TUPLE                   ::std::tuple
#define MA_TUPLE_GET               ::std::get
#define MA_MAKE_TUPLE              ::std::make_tuple

#else // defined(MA_USE_CXX11_STDLIB)

#define MA_SHARED_PTR              ::boost::shared_ptr
#define MA_WEAK_PTR                ::boost::weak_ptr
#define MA_MAKE_SHARED             ::boost::make_shared
#define MA_ENABLE_SHARED_FROM_THIS ::boost::enable_shared_from_this
#define MA_BIND                    ::boost::bind
#define MA_PLACEHOLDER_1           _1
#define MA_PLACEHOLDER_2           _2
#define MA_REF                     ::boost::ref
#define MA_FUNCTION                ::boost::function
#define MA_ADDRESS_OF              ::boost::addressof
#define MA_STATIC_POINTER_CAST     ::boost::static_pointer_cast
#define MA_TUPLE                   ::boost::tuple
#define MA_TUPLE_GET               ::boost::get
#define MA_MAKE_TUPLE              ::boost::make_tuple

#endif // defined(MA_USE_CXX11_STDLIB)

#endif // MA_CONFIG_HPP
