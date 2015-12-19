//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WORK_BOUND_HANDLER_HPP
#define MA_WORK_BOUND_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#include <ma/type_traits.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler>
class work_bound_handler
{
private:
  typedef work_bound_handler<Handler> this_type;

public:
  typedef void result_type;

  template <typename H>
  work_bound_handler(boost::asio::io_service& io_service, MA_FWD_REF(H) handler)
    : handler_(detail::forward<H>(handler))
    , work_(io_service)
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  work_bound_handler(this_type&& other)
    : handler_(detail::move(other.handler_))
    , work_(detail::move(other.work_))
  {
  }

  work_bound_handler(const this_type& other)
    : handler_(other.handler_)
    , work_(other.work_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~work_bound_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    // Forward to asio_handler_allocate provided by the specified allocation
    // context.
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size, 
      this_type* context)
  {
    // Forward to asio_handler_deallocate provided by the specified allocation
    // context.
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
  Handler handler_;
  boost::asio::io_service::work work_;
}; // class work_bound_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

/// Helper for creation of handler.
template <typename Handler>
inline work_bound_handler<typename remove_cv_reference<Handler>::type>
make_work_bound_handler(boost::asio::io_service& io_service,
    MA_FWD_REF(Handler) handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  return work_bound_handler<handler_type>(
      io_service, detail::forward<Handler>(handler));
}

} // namespace ma

#endif // MA_WORK_BOUND_HANDLER_HPP
