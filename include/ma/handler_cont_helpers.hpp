//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/utility/addressof.hpp>

namespace ma_handler_cont_helpers {

#if BOOST_VERSION >= 105400

template <typename Context>
inline bool is_continuation(Context& context)
{
  using namespace boost::asio;
  return asio_handler_is_continuation(boost::addressof(context));
}

#else  // BOOST_VERSION >= 105400

template <typename Context>
inline bool is_continuation(Context& /*context*/)
{
  return false;
}

#endif // BOOST_VERSION >= 105400

} // namespace ma_handler_cont_helpers

#endif // MA_HANDLER_CONT_HELPERS_HPP
