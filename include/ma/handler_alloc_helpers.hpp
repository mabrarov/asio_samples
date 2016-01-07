//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
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
#include <ma/config.hpp>
#include <ma/detail/memory.hpp>

// Calls to asio_handler_allocate and asio_handler_deallocate must be made from
// a namespace that does not contain any overloads of these functions. The
// ma_handler_alloc_helpers namespace is defined here for that purpose.
// It's a modified copy of Asio sources: asio/detail/handler_alloc_helpers.hpp
namespace ma_handler_alloc_helpers {

namespace detail {

using namespace boost::asio;

template <typename Context>
struct noexcept_traits
{
  static void allocate()
      MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(asio_handler_allocate(
          static_cast<std::size_t>(0), static_cast<Context*>(0))))
  {
    // do nothing
  }

  static void deallocate()
      MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(asio_handler_deallocate(
          static_cast<void*>(0), static_cast<std::size_t>(0),
          static_cast<Context*>(0))))
  {
    // do nothing
  }
}; // struct noexcept_traits

} // namespace detail

template <typename Context>
inline void* allocate(std::size_t size, Context& context)
    MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
        detail::noexcept_traits<Context>::allocate()))
{
  using namespace boost::asio;
  return asio_handler_allocate(size, ma::detail::addressof(context));
}

template <typename Context>
inline void deallocate(void* pointer, std::size_t size, Context& context)
    MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
        detail::noexcept_traits<Context>::deallocate()))
{
  using namespace boost::asio;
  asio_handler_deallocate(pointer, size, ma::detail::addressof(context));
}

} // namespace ma_handler_alloc_helpers

namespace ma {
namespace detail {

template <typename Context>
struct context_alloc_noexcept_traits
{
  static void allocate() MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
      ma_handler_alloc_helpers::allocate(static_cast<std::size_t>(0),
          *(static_cast<Context*>(0)))))
  {
    // do nothing
  }

  static void deallocate() MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
      ma_handler_alloc_helpers::deallocate(static_cast<void*>(0),
          static_cast<std::size_t>(0), *(static_cast<Context*>(0)))))
  {
    // do nothing
  }
}; // struct context_alloc_noexcept_traits

} // namespace detail
} // namespace ma


#endif // MA_HANDLER_ALLOC_HELPERS_HPP
