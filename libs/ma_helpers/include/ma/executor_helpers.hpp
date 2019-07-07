//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_EXECUTOR_HELPERS_HPP
#define MA_EXECUTOR_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <ma/config.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

#if BOOST_VERSION < 107000

template <typename IoObject>
boost::asio::io_service& get_executor(IoObject& io_object)
{
  return io_object.get_io_service();
}

template <typename Handler>
void post(boost::asio::io_service& io_service, MA_FWD_REF(Handler) handler)
{
  io_service.post(ma::detail::forward<Handler>(handler));
}

#else // BOOST_VERSION < 107000

template <typename IoObject>
typename IoObject::executor_type get_executor(IoObject& io_object)
{
  return io_object.get_executor();
}

using boost::asio::post;

#endif // BOOST_VERSION < 107000

} // namespace ma

#endif // MA_EXECUTOR_HELPERS_HPP
