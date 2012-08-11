//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONTEXT_WRAPPED_HANDLER_HPP
#define MA_CONTEXT_WRAPPED_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

/// Wrappers that override allocation/execution strategies of the source
/// handler.
/**
 * "Alloctaion strategy" means handler related pair of free functions:
 * asio_handler_allocate and asio_handler_deallocate or the default ones
 * defined by Asio.
 * http://www.boost.org/doc/libs/1_50_0/doc/html/boost_asio/reference/Handler.html
 *
 * "Execution strategy" means handler related free function asio_handler_invoke
 * or the default one defined by Asio.
 * http://www.boost.org/doc/libs/1_50_0/doc/html/boost_asio/reference/Handler.html
 *
 * Functors created by listed wrappers:
 *
 * @li override Asio alloctaion strategy to the one provided by context
 * parameter.
 * @li override Asio execution strategy to the one provided by handler
 * parameter.
 * @li forward operator() to to the ones provided by handler parameter.
 *
 * The handler parameter must meet the requirements of Asio handler.
 * The context parameter must meet the requirements of Asio handler except
 * existance of operator() - it isn't applied to context parameter.
 * The functors created by means of listed wrappers meet the requirements of
 * Asio handler.
 *
 * Usage of free functions called make_context_wrapped_handler and
 * make_explicit_context_wrapped_handler can help in construction of functors.
 *
 * It's a much modified copy of Boost.Asio sources:
 * asio/detail/wrapped_handler.hpp (rewrapped_handler). The reason of copy is
 * that those sources are in private area of Boost.Asio.
 *
 * Move semantic supported.
 * Move constructor is explicitly defined to support MSVC 2010.
 */

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Context, typename Handler>
class context_wrapped_handler
{
private:
  typedef context_wrapped_handler<Context, Handler> this_type;

public:
  typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)

  template <typename C, typename H>
  context_wrapped_handler(C&& context, H&& handler)
    : context_(std::forward<C>(context))
    , handler_(std::forward<H>(handler))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  context_wrapped_handler(this_type&& other)
    : context_(std::move(other.context_))
    , handler_(std::move(other.handler_))
  {
  }

  context_wrapped_handler(const this_type& other)
    : context_(other.context_)
    , handler_(other.handler_)
  {
  }

#endif

#else // defined(MA_HAS_RVALUE_REFS)

  context_wrapped_handler(const Context& context, const Handler& handler)
    : context_(context)
    , handler_(handler)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~context_wrapped_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_asio_handler_alloc_helpers::deallocate(pointer, size,
        context->context_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function&& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(std::forward<Function>(function),
        context->context_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(function, context->context_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  void operator()()
  {
    handler_();
  }

  template <typename Arg1>
  void operator()(const Arg1& arg1)
  {
    handler_(arg1);
  }

  template <typename Arg1, typename Arg2>
  void operator()(const Arg1& arg1, const Arg2& arg2)
  {
    handler_(arg1, arg2);
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
  {
    handler_(arg1, arg2, arg3);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4)
  {
    handler_(arg1, arg2, arg3, arg4);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4, const Arg5& arg5)
  {
    handler_(arg1, arg2, arg3, arg4, arg5);
  }

  void operator()() const
  {
    handler_();
  }

  template <typename Arg1>
  void operator()(const Arg1& arg1) const
  {
    handler_(arg1);
  }

  template <typename Arg1, typename Arg2>
  void operator()(const Arg1& arg1, const Arg2& arg2) const
  {
    handler_(arg1, arg2);
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
  {
    handler_(arg1, arg2, arg3);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4) const
  {
    handler_(arg1, arg2, arg3, arg4);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4, const Arg5& arg5) const
  {
    handler_(arg1, arg2, arg3, arg4, arg5);
  }

private:
  Context context_;
  Handler handler_;
}; // class context_wrapped_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Context, typename Handler>
inline context_wrapped_handler<
    typename remove_cv_reference<Context>::type,
    typename remove_cv_reference<Handler>::type>
make_context_wrapped_handler(Context&& context, Handler&& handler)
{
  typedef typename remove_cv_reference<Context>::type context_type;
  typedef typename remove_cv_reference<Handler>::type handler_type;
  return context_wrapped_handler<context_type, handler_type>(
      std::forward<Context>(context), std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Context, typename Handler>
inline context_wrapped_handler<Context, Handler>
make_context_wrapped_handler(const Context& context, const Handler& handler)
{
  return context_wrapped_handler<Context, Handler>(context, handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Context, typename Handler>
class explicit_context_wrapped_handler
{
private:
  typedef explicit_context_wrapped_handler<Context, Handler> this_type;

public:
  typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)

  template <typename C, typename H>
  explicit_context_wrapped_handler(C&& context, H&& handler)
    : context_(std::forward<C>(context))
    , handler_(std::forward<H>(handler))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  explicit_context_wrapped_handler(this_type&& other)
    : context_(std::move(other.context_))
    , handler_(std::move(other.handler_))
  {
  }

  explicit_context_wrapped_handler(const this_type& other)
    : context_(other.context_)
    , handler_(other.handler_)
  {
  }

#endif

#else // defined(MA_HAS_RVALUE_REFS)

  explicit_context_wrapped_handler(const Context& context,
      const Handler& handler)
    : context_(context)
    , handler_(handler)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~explicit_context_wrapped_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_asio_handler_alloc_helpers::deallocate(pointer, size,
        context->context_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function&& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(std::forward<Function>(function),
        context->context_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(function, context->context_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  void operator()()
  {
    handler_(static_cast<const Context&>(context_));
  }

  template <typename Arg1>
  void operator()(const Arg1& arg1)
  {
    handler_(static_cast<const Context&>(context_), arg1);
  }

  template <typename Arg1, typename Arg2>
  void operator()(const Arg1& arg1, const Arg2& arg2)
  {
    handler_(static_cast<const Context&>(context_), arg1, arg2);
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
  {
    handler_(static_cast<const Context&>(context_), arg1, arg2, arg3);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4)
  {
    handler_(static_cast<const Context&>(context_), arg1, arg2, arg3, arg4);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4, const Arg5& arg5)
  {
    handler_(static_cast<const Context&>(context_), arg1, arg2, arg3, arg4,
        arg5);
  }

  void operator()() const
  {
    handler_(context_);
  }

  template <typename Arg1>
  void operator()(const Arg1& arg1) const
  {
    handler_(context_, arg1);
  }

  template <typename Arg1, typename Arg2>
  void operator()(const Arg1& arg1, const Arg2& arg2) const
  {
    handler_(context_, arg1, arg2);
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) const
  {
    handler_(context_, arg1, arg2, arg3);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4) const
  {
    handler_(context_, arg1, arg2, arg3, arg4);
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
      const Arg4& arg4, const Arg5& arg5) const
  {
    handler_(context_, arg1, arg2, arg3, arg4, arg5);
  }

private:
  Context context_;
  Handler handler_;
}; // class explicit_context_wrapped_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Context, typename Handler>
inline explicit_context_wrapped_handler<
    typename remove_cv_reference<Context>::type,
    typename remove_cv_reference<Handler>::type>
make_explicit_context_wrapped_handler(Context&& context, Handler&& handler)
{
  typedef typename remove_cv_reference<Context>::type context_type;
  typedef typename remove_cv_reference<Handler>::type handler_type;
  return explicit_context_wrapped_handler<context_type, handler_type>(
      std::forward<Context>(context), std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Context, typename Handler>
inline explicit_context_wrapped_handler<Context, Handler>
make_explicit_context_wrapped_handler(const Context& context,
    const Handler& handler)
{
  return explicit_context_wrapped_handler<Context, Handler>(context, handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

} // namespace ma

#endif // MA_CONTEXT_WRAPPED_HANDLER_HPP
