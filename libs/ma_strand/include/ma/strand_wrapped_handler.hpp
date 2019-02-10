//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_STRAND_WRAPPED_HANDLER_HPP
#define MA_STRAND_WRAPPED_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#include <ma/context_wrapped_handler.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

#if defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

/// The wrapper to use with asio::io_service::strand.
/**
 * strand_wrapped_handler creates handler that works similar to the one created
 * by asio::io_service::strand::wrap except the guarantee given by Asio:
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/io_service__strand/wrap.html
 * ...
 * that, when invoked, executes code equivalent to:
 *   strand.dispatch(bind(f, a1, ... an));
 * ...
 * strand_wrapped_handler does strand.dispatch only when it is called by the
 * means of asio_handler_invoke. See MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER
 * in ma/config.hpp for details.
 *
 * Note that strand_wrapped_handler (like the one created by
 * asio::io_service::strand::wrap) doesn't simply override-with-replacement the
 * execution strategy of the source handler (like context_wrapped_handler
 * does). strand_wrapped_handler adds strand related execution strategy before
 * the one provided by source handler.
 *
 * "Execution strategy" means handler related free function asio_handler_invoke
 * or the default one defined by Asio.
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 */

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler>
class strand_wrapped_handler
{
private:
  typedef strand_wrapped_handler<Handler> this_type;

public:
  typedef void result_type;

  template <typename H>
  strand_wrapped_handler(boost::asio::io_service::strand& strand,
      MA_FWD_REF(H) handler)
    : strand_(detail::addressof(strand))
    , handler_(detail::forward<H>(handler))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  strand_wrapped_handler(this_type&& other)
    : strand_(other.strand_)
    , handler_(detail::move(other.handler_))
  {
#if !defined(NDEBUG)
    // For the check of usage of asio invocation.
    other.strand_ = 0;
#endif
  }

  strand_wrapped_handler(const this_type& other)
    : strand_(other.strand_)
    , handler_(other.handler_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~strand_wrapped_handler()
  {
    // For the check of usage of asio invocation.
    strand_ = 0;
  }
#endif

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
    boost::asio::io_service::strand& strand = *context->strand_;
    strand.dispatch(make_context_wrapped_handler(context->handler_,
        detail::forward<Function>(function)));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    boost::asio::io_service::strand& strand = *context->strand_;
    strand.dispatch(make_context_wrapped_handler(context->handler_, function));
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    boost::asio::io_service::strand& strand = *context->strand_;
    strand.dispatch(make_context_wrapped_handler(context->handler_, function));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if BOOST_VERSION >= 105400

  friend bool asio_handler_is_continuation(this_type* context)
  {
    boost::asio::io_service::strand& strand = *context->strand_;
    return strand.running_in_this_thread();
  }

#endif // BOOST_VERSION >= 105400

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
  boost::asio::io_service::strand* strand_;
  Handler handler_;
}; // class strand_wrapped_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

template <typename Handler>
inline strand_wrapped_handler<typename detail::decay<Handler>::type>
make_strand_wrapped_handler(boost::asio::io_service::strand& strand,
    MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  return strand_wrapped_handler<handler_type>(strand,
      detail::forward<Handler>(handler));
}

#endif // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

} // namespace ma

#endif // MA_STRAND_WRAPPED_HANDLER_HPP
