//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CUSTOM_ALLOC_HANDLER_HPP
#define MA_CUSTOM_ALLOC_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

/// Wrapper that overrides allocation strategy of the source handler by the
/// means provided by specified handler allocator.
/**
 * "Allocation strategy" means handler related pair of free functions:
 * asio_handler_allocate and asio_handler_deallocate or the default ones
 * defined by Asio.
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 *
 * "Execution strategy" means handler related free function asio_handler_invoke
 * or the default one defined by Asio.
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 *
 * Functors created by custom_alloc_handler:
 *
 * @li override Asio allocation strategy by the means provided by specified
 * handler allocator.
 * @li forward Asio execution strategy to the one provided by handler
 * parameter.
 * @li forward operator() to to the ones provided by handler parameter.
 *
 * The handler parameter must meet the requirements of Asio handler.
 * The allocator parameter must provide member functions:
 *
 * @li void* allocate(std::size_t size)
 * @li void deallocate(void* pointer)
 *
 * See ma/handler_allocator.hpp for implementations of handler allocator.
 *
 * The functors created by means of custom_alloc_handler meet the requirements
 * of Asio handler and store only a pointer to the specified allocator. So the
 * source handler is responsible for the availability (life time) of the
 * specified handler allocator.
 *
 * Usage of free function make_custom_alloc_handler can help in construction of
 * functors.
 *
 * custom_alloc_handler is very similar to context_alloc_handler with such
 * differences:
 *
 * @li context_alloc_handler makes and stores copy of context and
 * custom_alloc_handler stores only pointer to provided handler allocator.
 * @li custom_alloc_handler has additional debug check against an error found
 * in some of the Boost.Asio versions (older than Boost 1.46). See
 * http://asio-samples.blogspot.com/2010/07/chris.html for details.
 * @li custom_alloc_handler can be replaced with context_alloc_handler which is
 * more general. But such a replacement won't provide debug check and will
 * require an explicit pass of a pointer to the handler allocator.
 *
 * Move semantic supported.
 * Move constructor is explicitly defined to support MSVC 2010.
 */

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Allocator, typename Handler>
class custom_alloc_handler
{
private:
  typedef custom_alloc_handler<Allocator, Handler> this_type;

public:
  typedef void result_type;

#if !defined(NDEBUG)
  ~custom_alloc_handler()
  {
    // For the check of usage of asio custom memory allocation.
    allocator_ = 0;
  }
#endif

  template <typename H>
  custom_alloc_handler(Allocator& allocator, MA_FWD_REF(H) handler)
    : allocator_(detail::addressof(allocator))
    , handler_(detail::forward<H>(handler))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  custom_alloc_handler(this_type&& other)
    : allocator_(other.allocator_)
    , handler_(detail::move(other.handler_))
  {
#if !defined(NDEBUG)
    // For the check of usage of asio invocation.
    other.allocator_ = 0;
#endif
  }

  custom_alloc_handler(const this_type& other)
    : allocator_(other.allocator_)
    , handler_(other.handler_)
  {
  }

#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    if (void* ptr = context->allocator_->allocate(size))
    {
      return ptr;
    }
    else
    {
      return ma_handler_alloc_helpers::allocate(size, context->handler_);
    }
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    if (context->allocator_->owns(pointer))
    {
      context->allocator_->deallocate(pointer);
    }
    else
    {
      ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
    }
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

  void operator()()
  {
    handler_();
  }

  template <typename Arg1>
  void operator()(MA_FWD_REF(Arg1) arg1)
  {
    handler_(detail::forward<Arg1>(arg1));
  }

  template <typename Arg1, typename Arg2>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2)
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2));
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3)
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4)
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4, MA_FWD_REF(Arg5) arg5)
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4),
        detail::forward<Arg5>(arg5));
  }

  void operator()() const
  {
    handler_();
  }

  template <typename Arg1>
  void operator()(MA_FWD_REF(Arg1) arg1) const
  {
    handler_(detail::forward<Arg1>(arg1));
  }

  template <typename Arg1, typename Arg2>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2) const
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2));
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3) const
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4) const
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4, MA_FWD_REF(Arg5) arg5) const
  {
    handler_(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
        detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4),
        detail::forward<Arg5>(arg5));
  }

private:
  Allocator* allocator_;
  Handler    handler_;
}; // class custom_alloc_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

template <typename Allocator, typename Handler>
inline custom_alloc_handler<Allocator,
    typename detail::decay<Handler>::type>
make_custom_alloc_handler(Allocator& allocator, MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  return custom_alloc_handler<Allocator, handler_type>(allocator,
      detail::forward<Handler>(handler));
}

} // namespace ma

#endif // MA_CUSTOM_ALLOC_HANDLER_HPP
