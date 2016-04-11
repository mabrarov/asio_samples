//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_BINDER_HPP
#define MA_DETAIL_BINDER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/utility.hpp>

namespace ma {
namespace detail {

/// Provides binders with support of Asio specificity.
/**
 * Functors created by listed binders forward Asio allocation/execution
 * strategies to the ones provided by source handler.
 *
 * "Allocation strategy" means handler related pair of free functions:
 * asio_handler_allocate and asio_handler_deallocate or the default ones
 * defined by Asio.
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 *
 * "Execution strategy" means handler related free function asio_handler_invoke
 * or the default one defined by Asio.
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 *
 * The source handler must meet the requirements of Asio handler.
 * The bound arguments must meet the requirements of Asio handler except
 * existence of asio_handler_allocate, asio_handler_deallocate,
 * asio_handler_invoke and operator() - these functions aren't applied to
 * bound arguments.
 * The functors created by means of listed binders meet the requirements of
 * Asio handler.
 *
 * Usage of free functions called ma::bind_handler can help in construction of
 * functors.
 *
 * It's a modified copy of Boost.Asio sources: asio/detail/bind_handler.hpp.
 * The reason of copy is that those sources are in private area of Boost.Asio.
 *
 * Move semantic supported.
 * Move constructor is explicitly defined to support MSVC 2010/2012.
 */

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg1>
class binder1
{
private:
  typedef binder1<Handler, Arg1> this_type;

public:
  typedef void result_type;

  template <typename H, typename A1>
  binder1(MA_FWD_REF(H) handler, MA_FWD_REF(A1) arg1)
    : handler_(detail::forward<H>(handler))
    , arg1_(detail::forward<A1>(arg1))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  binder1(this_type&& other)
    : handler_(detail::move(other.handler_))
    , arg1_(detail::move(other.arg1_))
  {
  }

  binder1(const this_type& other)
    : handler_(other.handler_)
    , arg1_(other.arg1_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~binder1()
  {
  }
#endif

  void operator()()
  {
    handler_(arg1_);
  }

  void operator()() const
  {
    handler_(arg1_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    // Forward to asio_handler_allocate provided by source handler.
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    // Forward to asio_handler_deallocate provided by source handler.
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    // Forward to asio_handler_invoke provided by source handler.
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    // Forward to asio_handler_invoke provided by source handler.
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    // Forward to asio_handler_invoke provided by source handler.
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

private:
  Handler handler_;
  Arg1 arg1_;
}; // class binder1

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg1, typename Arg2>
class binder2
{
private:
  typedef binder2<Handler, Arg1, Arg2> this_type;

public:
  typedef void result_type;

  template <typename H, typename A1, typename A2>
  binder2(MA_FWD_REF(H) handler, MA_FWD_REF(A1) arg1, MA_FWD_REF(A2) arg2)
    : handler_(detail::forward<H>(handler))
    , arg1_(detail::forward<A1>(arg1))
    , arg2_(detail::forward<A2>(arg2))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  binder2(this_type&& other)
    : handler_(detail::move(other.handler_))
    , arg1_(detail::move(other.arg1_))
    , arg2_(detail::move(other.arg2_))
  {
  }

  binder2(const this_type& other)
    : handler_(other.handler_)
    , arg1_(other.arg1_)
    , arg2_(other.arg2_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~binder2()
  {
  }
#endif

  void operator()()
  {
    handler_(arg1_, arg2_);
  }

  void operator()() const
  {
    handler_(arg1_, arg2_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
}; // class binder2

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
class binder3
{
private:
  typedef binder3<Handler, Arg1, Arg2, Arg3> this_type;

public:
  typedef void result_type;

  template <typename H, typename A1, typename A2, typename A3>
  binder3(MA_FWD_REF(H) handler, MA_FWD_REF(A1) arg1, MA_FWD_REF(A2) arg2,
      MA_FWD_REF(A3) arg3)
    : handler_(detail::forward<H>(handler))
    , arg1_(detail::forward<A1>(arg1))
    , arg2_(detail::forward<A2>(arg2))
    , arg3_(detail::forward<A3>(arg3))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  binder3(this_type&& other)
    : handler_(detail::move(other.handler_))
    , arg1_(detail::move(other.arg1_))
    , arg2_(detail::move(other.arg2_))
    , arg3_(detail::move(other.arg3_))
  {
  }

  binder3(const this_type& other)
    : handler_(other.handler_)
    , arg1_(other.arg1_)
    , arg2_(other.arg2_)
    , arg3_(other.arg3_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~binder3()
  {
  }
#endif

  void operator()()
  {
    handler_(arg1_, arg2_, arg3_);
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
}; // class binder3

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4>
class binder4
{
private:
  typedef binder4<Handler, Arg1, Arg2, Arg3, Arg4> this_type;

public:
  typedef void result_type;

  template <typename H, typename A1, typename A2, typename A3, typename A4>
  binder4(MA_FWD_REF(H) handler, MA_FWD_REF(A1) arg1, MA_FWD_REF(A2) arg2,
      MA_FWD_REF(A3) arg3, MA_FWD_REF(A4) arg4)
    : handler_(detail::forward<H>(handler))
    , arg1_(detail::forward<A1>(arg1))
    , arg2_(detail::forward<A2>(arg2))
    , arg3_(detail::forward<A3>(arg3))
    , arg4_(detail::forward<A4>(arg4))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  binder4(this_type&& other)
    : handler_(detail::move(other.handler_))
    , arg1_(detail::move(other.arg1_))
    , arg2_(detail::move(other.arg2_))
    , arg3_(detail::move(other.arg3_))
    , arg4_(detail::move(other.arg4_))
  {
  }

  binder4(const this_type& other)
    : handler_(other.handler_)
    , arg1_(other.arg1_)
    , arg2_(other.arg2_)
    , arg3_(other.arg3_)
    , arg4_(other.arg4_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~binder4()
  {
  }
#endif

  void operator()()
  {
    handler_(arg1_, arg2_, arg3_, arg4_);
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_, arg4_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else //defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
  Arg4 arg4_;
}; // class binder4

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4, typename Arg5>
class binder5
{
private:
  typedef binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;

public:
  typedef void result_type;

  template <typename H, typename A1, typename A2, typename A3, typename A4,
      typename A5>
  binder5(MA_FWD_REF(H) handler, MA_FWD_REF(A1) arg1, MA_FWD_REF(A2) arg2,
      MA_FWD_REF(A3) arg3, MA_FWD_REF(A4) arg4, MA_FWD_REF(A5) arg5)
    : handler_(detail::forward<H>(handler))
    , arg1_(detail::forward<A1>(arg1))
    , arg2_(detail::forward<A2>(arg2))
    , arg3_(detail::forward<A3>(arg3))
    , arg4_(detail::forward<A4>(arg4))
    , arg5_(detail::forward<A5>(arg5))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  binder5(this_type&& other)
    : handler_(detail::move(other.handler_))
    , arg1_(detail::move(other.arg1_))
    , arg2_(detail::move(other.arg2_))
    , arg3_(detail::move(other.arg3_))
    , arg4_(detail::move(other.arg4_))
    , arg5_(detail::move(other.arg5_))
  {
  }

  binder5(const this_type& other)
    : handler_(other.handler_)
    , arg1_(other.arg1_)
    , arg2_(other.arg2_)
    , arg3_(other.arg3_)
    , arg4_(other.arg4_)
    , arg5_(other.arg5_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~binder5()
  {
  }
#endif

  void operator()()
  {
    handler_(arg1_, arg2_, arg3_, arg4_, arg5_);
  }

  void operator()() const
  {
    handler_(arg1_, arg2_, arg3_, arg4_, arg5_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

private:
  Handler handler_;
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;
  Arg4 arg4_;
  Arg5 arg5_;
}; // class binder5

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_BINDER_HPP
