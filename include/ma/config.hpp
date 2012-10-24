//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
#if defined(BOOST_HAS_RVALUE_REFS)

/// Turns on move semantic support.
#define MA_HAS_RVALUE_REFS

/// Defines has the functors created by means of boost::bind move constructor
/// (explicit or implicit).
/**
 * If functors created by means of boost::bind have no move constructor then
 * some of the asio-samples explicitly define and use binders with (explicit or
 * implicit - see MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) move constructor.
 */
#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)

/// Turns off explicit definition of move constructor (and copy constructor).
#undef MA_USE_EXPLICIT_MOVE_CONSTRUCTOR
/// Turns off usage of home-grown binders with move semantic support.
#undef MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#else

/// Turns on explicit definition of move constructor (and copy constructor).
#define MA_USE_EXPLICIT_MOVE_CONSTRUCTOR
/// Turns on usage of home-grown binders with move semantic support.
#define MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif

#else // defined(BOOST_HAS_RVALUE_REFS)

#undef  MA_HAS_RVALUE_REFS
#undef  MA_USE_EXPLICIT_MOVE_CONSTRUCTOR
#define MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR

#endif // defined(BOOST_HAS_RVALUE_REFS)

/// Defines does asio::io_service::strand::wrap produce "heavy" functor.
/**
 * Because of the guarantee given by Asio:
 * http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/reference/io_service__strand/wrap.html
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

#if BOOST_VERSION >= 104700

// BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG implies Boost.Thread to be rebuilt 
// i.e. custom (user) configuration of Boost C++ Libraries must be defined. 
// Add #define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG to <boost/user/config.hpp>
// and rebuild Boost (or all Boost libs using Boost.DateTime).
/// Turns on usage of Boost.Chrono at ma::steady_deadline_timer implementation.
#define MA_HAS_BOOST_CHRONO

#else

/// ma::steady_deadline_timer equals to boost::asio::deadline_timer
#undef MA_HAS_BOOST_CHRONO

#endif // BOOST_VERSION >= 104700

#if BOOST_VERSION >= 104800

/// Turns on usage of Boost.Timer.
#define MA_HAS_BOOST_TIMER

#else

#undef MA_HAS_BOOST_TIMER

#endif // BOOST_VERSION >= 104800

// Don't use vurtual functions for type erasure
#undef MA_TYPE_ERASURE_USE_VURTUAL

#endif // MA_CONFIG_HPP
