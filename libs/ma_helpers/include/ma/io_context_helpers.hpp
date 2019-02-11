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
#include <ma/config.hpp>

#if defined(MA_ASIO_IO_CONTEXT_INT_CONCURRENCY_HINT)
#include <boost/asio.hpp>
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

} // namespace ma

#endif // MA_IO_CONTEXT_HELPERS_HPP
