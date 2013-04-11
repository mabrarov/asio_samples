//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_LOCKABLE_WRAPPED_HANDLER_HPP
#define MA_LOCKABLE_WRAPPED_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/utility/addressof.hpp>
#include <boost/thread/lock_guard.hpp>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

/// Wrappers that override execution strategy of the source handler 
/// providing lock guard by means of Lockable.

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Lockable, typename Handler>
class lockable_wrapped_handler
{
private:
  typedef lockable_wrapped_handler<Lockable, Handler> this_type;

public:
  typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)

  template <typename H>
  lockable_wrapped_handler(Lockable& lockable, H&& handler)
    : lockable_(boost::addressof(lockable))
    , handler_(std::forward<H>(handler))
  {
  }

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  lockable_wrapped_handler(this_type&& other)
    : lockable_(other.lockable_)
    , handler_(std::move(other.handler_))
  {
#if !defined(NDEBUG)
    // For the check of usage of asio invocation.
    other.lockable_ = 0;
#endif
  }

  lockable_wrapped_handler(const this_type& other)
    : lockable_(other.lockable_)
    , handler_(other.handler_)
  {
  }

#endif

#else // defined(MA_HAS_RVALUE_REFS)

  lockable_wrapped_handler(Lockable& lockable, const Handler& handler)
    : lockable_(boost::addressof(lockable))
    , handler_(handler)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~lockable_wrapped_handler()
  {
    // For the check of usage of asio invocation.
    lockable_ = 0;
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    // Forward to asio_handler_allocate provided by the specified allocation
    // context.
    return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size, 
      this_type* context)
  {
    // Forward to asio_handler_deallocate provided by the specified allocation
    // context.
    ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function&& function, this_type* context)
  {
    // Acquire lock
    Lockable& lockable = *context->lockable_;
    boost::lock_guard<Lockable> lock_guard(lockable);
    // Forward to asio_handler_invoke provided by source handler.
    ma_asio_handler_invoke_helpers::invoke(std::forward<Function>(function),
        context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    // Acquire lock
    Lockable& lockable = *context->lockable_;
    boost::lock_guard<Lockable> lock_guard(lockable);
    // Forward to asio_handler_invoke provided by source handler.
    ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
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
  Lockable* lockable_;
  Handler handler_;
}; // class lockable_wrapped_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(MA_HAS_RVALUE_REFS)

/// Helper for creation of wrapped handler.
template <typename Lockable, typename Handler>
inline lockable_wrapped_handler<
    typename remove_cv_reference<Lockable>::type,
    typename remove_cv_reference<Handler>::type>
make_lockable_wrapped_handler(Lockable&& lockable, Handler&& handler)
{
  typedef typename remove_cv_reference<Lockable>::type lockable_type;
  typedef typename remove_cv_reference<Handler>::type  handler_type;
  return lockable_wrapped_handler<lockable_type, handler_type>(
      std::forward<Lockable>(lockable), std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

/// Helper for creation of wrapped handler.
template <typename Lockable, typename Handler>
inline lockable_wrapped_handler<Lockable, Handler>
make_lockable_wrapped_handler(Lockable& lockable, const Handler& handler)
{
  return lockable_wrapped_handler<Lockable, Handler>(lockable, handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

} // namespace ma

#endif // MA_LOCKABLE_WRAPPED_HANDLER_HPP
