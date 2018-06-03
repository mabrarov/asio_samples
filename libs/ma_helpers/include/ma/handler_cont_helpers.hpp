//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_CONT_HELPERS_HPP
#define MA_HANDLER_CONT_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <ma/config.hpp>
#include <ma/detail/memory.hpp>

namespace ma_handler_cont_helpers {

namespace detail {

using namespace boost::asio;

#if BOOST_VERSION >= 105400

template <typename Context>
inline bool is_continuation(Context& context) MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
    asio_handler_is_continuation(ma::detail::addressof(context))))
{
  return asio_handler_is_continuation(ma::detail::addressof(context));
}

#else  // BOOST_VERSION >= 105400

template <typename Context>
inline bool is_continuation(Context& /*context*/) MA_NOEXCEPT
{
  return false;
}

#endif // BOOST_VERSION >= 105400

} // namespace detail

using detail::is_continuation;

} // namespace ma_handler_cont_helpers

#endif // MA_HANDLER_CONT_HELPERS_HPP
