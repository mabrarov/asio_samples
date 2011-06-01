//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/context_wrapped_handler.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma
{    
#if defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)
  template <typename Handler>
  class strand_wrapped_handler
  {
  private:
    typedef strand_wrapped_handler<Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)
    template <typename H>
    strand_wrapped_handler(boost::asio::io_service::strand& strand, H&& handler)
      : strand_(strand)
      , handler_(std::forward<H>(handler))
    {
    }

    strand_wrapped_handler(this_type&& other)
      : strand_(other.strand_)
      , handler_(std::move(other.handler_))
    {
    }
#else
    strand_wrapped_handler(boost::asio::io_service::strand& strand, const Handler& handler)
      : strand_(strand)
      , handler_(handler)
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~strand_wrapped_handler()
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
    }

#if defined(MA_HAS_RVALUE_REFS)
    template <typename Function>
    friend void asio_handler_invoke(Function&& function, this_type* context)
    {
      context->strand_.dispatch(make_context_wrapped_handler(context->handler_, std::forward<Function>(function)));
    }
#else
    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      context->strand_.dispatch(make_context_wrapped_handler(context->handler_, function));
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
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    {
      handler_(arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
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
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) const
    {
      handler_(arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) const
    {
      handler_(arg1, arg2, arg3, arg4, arg5);
    }

  private:
    boost::asio::io_service::strand& strand_;
    Handler handler_;
  }; //class strand_wrapped_handler

#if defined(MA_HAS_RVALUE_REFS)
  template <typename Handler>
  inline strand_wrapped_handler<typename ma::remove_cv_reference<Handler>::type>
  make_strand_wrapped_handler(boost::asio::io_service::strand& strand, Handler&& handler)
  {    
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    return strand_wrapped_handler<handler_type>(strand, std::forward<Handler>(handler));
  }
#else
  template <typename Handler>
  inline strand_wrapped_handler<Handler> 
  make_strand_wrapped_handler(boost::asio::io_service::strand& strand, const Handler& handler)
  {
    return strand_wrapped_handler<Handler>(strand, handler);
  } // make_strand_wrapped_handler
#endif // defined(MA_HAS_RVALUE_REFS)

#define MA_STRAND_WRAP(strand, handler) (make_strand_wrapped_handler((strand), (handler)))

#else // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)

#define MA_STRAND_WRAP(strand, handler) ((strand).wrap(handler))

#endif // defined(MA_BOOST_ASIO_HEAVY_STRAND_WRAPPED_HANDLER)
 
} //namespace ma

#endif // MA_STRAND_WRAPPED_HANDLER_HPP