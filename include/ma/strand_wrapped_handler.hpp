//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_STRAND_WRAPPED_HANDLER_HPP
#define MA_STRAND_WRAPPED_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/utility.hpp>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/context_wrapped_handler.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {    

#if defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

/// The wrapper to use with asio::io_service::strand.
/**
 * strand_wrapped_handler creates handler that works similar to the one created
 * by asio::io_service::strand::wrap except the guarantee given by Asio: 
 * http://www.boost.org/doc/libs/1_49_0/doc/html/boost_asio/reference/io_service__strand/wrap.html
 * ...
 * that, when invoked, executes code equivalent to: 
 *   strand.dispatch(boost::bind(f, a1, ... an)); 
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
 * http://www.boost.org/doc/libs/1_49_0/doc/html/boost_asio/reference/Handler.html
 *
 * Use MA_STRAND_WRAP macros to create a strand-wrapped handler according to
 * asio-samples configuration (MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER).
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

#if defined(MA_HAS_RVALUE_REFS)

  template <typename H>
  strand_wrapped_handler(boost::asio::io_service::strand& strand, H&& handler)
    : strand_(boost::addressof(strand))
    , handler_(std::forward<H>(handler))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  strand_wrapped_handler(this_type&& other)
    : strand_(other.strand_)
    , handler_(std::move(other.handler_))
  {
  }

  strand_wrapped_handler(const this_type& other)
    : strand_(other.strand_)
    , handler_(other.handler_)
  {
  }

#endif

#else // defined(MA_HAS_RVALUE_REFS)

  strand_wrapped_handler(boost::asio::io_service::strand& strand, 
      const Handler& handler)
    : strand_(boost::addressof(strand))
    , handler_(handler)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~strand_wrapped_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size, 
      this_type* context)
  {
    ma_asio_handler_alloc_helpers::deallocate(pointer, size, 
        context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function&& function, this_type* context)
  {
    context->strand_->dispatch(make_context_wrapped_handler(context->handler_,
        std::forward<Function>(function)));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    context->strand_->dispatch(make_context_wrapped_handler(context->handler_,
        function));
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
  boost::asio::io_service::strand* strand_;
  Handler handler_;
}; // class strand_wrapped_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
inline strand_wrapped_handler<typename ma::remove_cv_reference<Handler>::type>
make_strand_wrapped_handler(boost::asio::io_service::strand& strand, 
    Handler&& handler)
{    
  typedef typename ma::remove_cv_reference<Handler>::type handler_type;
  return strand_wrapped_handler<handler_type>(strand, 
      std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
inline strand_wrapped_handler<Handler> 
make_strand_wrapped_handler(boost::asio::io_service::strand& strand, 
    const Handler& handler)
{
  return strand_wrapped_handler<Handler>(strand, handler);
} // make_strand_wrapped_handler

#endif // defined(MA_HAS_RVALUE_REFS)

#define MA_STRAND_WRAP(strand, handler) \
    (::ma::make_strand_wrapped_handler((strand), (handler)))

#else // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

#define MA_STRAND_WRAP(strand, handler) ((strand).wrap(handler))

#endif // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)
 
} // namespace ma

#endif // MA_STRAND_WRAPPED_HANDLER_HPP
