//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_IO_CONTEXT_HELPERS_HPP
#define MA_IO_CONTEXT_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <ma/config.hpp>

#if defined(MA_ASIO_IO_CONTEXT_INT_CONCURRENCY_HINT)
#include <boost/numeric/conversion/cast.hpp>
#endif

namespace ma {

#if defined(MA_ASIO_IO_CONTEXT_INT_CONCURRENCY_HINT)
typedef int io_context_concurrency_hint;
#else
typedef std::size_t io_context_concurrency_hint;
#endif

inline io_context_concurrency_hint
to_io_context_concurrency_hint(std::size_t hint)
{
#if defined(MA_ASIO_IO_CONTEXT_INT_CONCURRENCY_HINT)
  if (1 == hint)
  {
    return BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO;
  }
  return boost::numeric_cast<io_context_concurrency_hint>(hint);
#else
  return hint;
#endif
}

inline boost::asio::io_service&
get_io_context(boost::asio::io_service::work& work)
{
#if BOOST_VERSION >= 107000
  return work.get_io_context();
#else
  return work.get_io_service();
#endif
}

inline boost::asio::io_service&
get_io_context(boost::asio::io_service::strand& strand)
{
#if BOOST_VERSION >= 107000
  return strand.context();
#else
  return strand.get_io_service();
#endif
}

inline boost::asio::io_service&
get_io_context(boost::asio::io_service::service& service)
{
#if BOOST_VERSION >= 107000
  return service.get_io_context();
#else
  return service.get_io_service();
#endif
}

} // namespace ma

#endif // MA_IO_CONTEXT_HELPERS_HPP
