//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOC_HELPERS_HPP
#define MA_HANDLER_ALLOC_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/utility/addressof.hpp>

// Calls to asio_handler_allocate and asio_handler_deallocate must be made from
// a namespace that does not contain any overloads of these functions. The
// ma_handler_alloc_helpers namespace is defined here for that purpose.
// It's a modified copy of Asio sources: asio/detail/handler_alloc_helpers.hpp
namespace ma_handler_alloc_helpers {

template <typename Context>
inline void* allocate(std::size_t size, Context& context)
{
  using namespace boost::asio;
  return asio_handler_allocate(size, boost::addressof(context)); //-V111
}

template <typename Context>
inline void deallocate(void* pointer, std::size_t size, Context& context)
{
  using namespace boost::asio;
  asio_handler_deallocate(pointer, size, boost::addressof(context)); //-V111
}

} // namespace ma_handler_alloc_helpers

#endif // MA_HANDLER_ALLOC_HELPERS_HPP
