//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
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
 * Listed options mostly provide additional optimizations.
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
#define MA_USE_CXX11_STDLIB_THREAD
#define MA_USE_CXX11_STDLIB_TYPE_TRAITS
#define MA_USE_CXX11_STDLIB_RANDOM

#elif (BOOST_VERSION >= 105500) && defined(BOOST_MSVC) && (BOOST_MSVC >= 1600)

#define MA_USE_CXX11_STDLIB_MEMORY
#define MA_USE_CXX11_STDLIB_TUPLE
#undef  MA_USE_CXX11_STDLIB_FUNCTIONAL
#undef  MA_USE_CXX11_STDLIB_THREAD
#define MA_USE_CXX11_STDLIB_TYPE_TRAITS
#define MA_USE_CXX11_STDLIB_RANDOM

#else

#undef  MA_USE_CXX11_STDLIB_MEMORY
#undef  MA_USE_CXX11_STDLIB_TUPLE
#undef  MA_USE_CXX11_STDLIB_FUNCTIONAL
#undef  MA_USE_CXX11_STDLIB_THREAD
#undef  MA_USE_CXX11_STDLIB_TYPE_TRAITS
#undef  MA_USE_CXX11_STDLIB_RANDOM

#endif

// Check the presence of r-value references support.
#if (BOOST_VERSION >= 104000) && !defined(BOOST_NO_RVALUE_REFERENCES)
#define MA_HAS_RVALUE_REFS
#else
#undef  MA_HAS_RVALUE_REFS
#endif

#if defined(MA_HAS_RVALUE_REFS)
#define MA_FWD_REF(T) T &&
#else
#define MA_FWD_REF(T) T const &
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
#undef MA_BIND_HAS_NO_MOVE_CONSTRUCTOR

#elif defined(BOOST_CLANG)

#if __has_feature(cxx_implicit_moves)

#undef MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#undef MA_BIND_HAS_NO_MOVE_CONSTRUCTOR

#else // __has_feature(cxx_implicit_moves)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONSTRUCTOR

#endif // __has_feature(cxx_implicit_moves)

#elif (defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)) \
    || (defined(BOOST_INTEL) && (BOOST_INTEL_CXX_VERSION >= 1400) \
        && defined(BOOST_INTEL_STDCXX0X))

#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1900)
// Starting from MSVS 2014 CTP Visual C+ supports implicit move constructors
#undef  MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#else
#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#endif // defined(BOOST_MSVC) && (BOOST_MSVC >= 1900)

#if defined(MA_USE_CXX11_STDLIB_FUNCTIONAL)
#undef  MA_BIND_HAS_NO_MOVE_CONSTRUCTOR
#else
#define MA_BIND_HAS_NO_MOVE_CONSTRUCTOR
#endif

#else  // defined(BOOST_GCC) && (BOOST_GCC >= 40600)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONSTRUCTOR

#endif // defined(BOOST_GCC) && (BOOST_GCC >= 40600)

#else  // defined(MA_HAS_RVALUE_REFS)

#define MA_NO_IMPLICIT_MOVE_CONSTRUCTOR
#define MA_BIND_HAS_NO_MOVE_CONSTRUCTOR

#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP) \
    && (BOOST_VERSION < 105600)
#undef  MA_BOOST_ASIO_WINDOWS_CONNECT_EX
#else
#define MA_BOOST_ASIO_WINDOWS_CONNECT_EX
#endif

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
#define MA_HAS_WINDOWS_CONSOLE_SIGNAL
#else
#undef  MA_HAS_WINDOWS_CONSOLE_SIGNAL
#endif

#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1500)

#if BOOST_MSVC < 1900
#define MA_NOEXCEPT throw()
#else
// MSVSC 2014 CTP or higher supports C++11 noexcept keyword
#define MA_NOEXCEPT noexcept
#endif

#elif defined(BOOST_NOEXCEPT)

#define MA_NOEXCEPT BOOST_NOEXCEPT

#else

#define MA_NOEXCEPT

#endif

#if !defined(MA_WIN32_TMAIN) && defined(WIN32) && !defined(__MINGW32__)
#define MA_WIN32_TMAIN
#endif

#endif // MA_CONFIG_HPP
