//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma
{    
  template <typename Allocator, typename Handler>
  class custom_alloc_handler
  {
  private:
    typedef custom_alloc_handler<Allocator, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)
    template <typename H>
    custom_alloc_handler(Allocator& allocator, H&& handler)
#if defined(_DEBUG)
      : allocator_(boost::addressof(allocator))
#else
      : allocator_(allocator)
#endif // defined(_DEBUG)  
      , handler_(std::forward<H>(handler))
    {
    }

    custom_alloc_handler(this_type&& other)
      : allocator_(other.allocator_)
      , handler_(std::move(other.handler_))
    {
    }
#else
    custom_alloc_handler(Allocator& allocator, const Handler& handler)
#if defined(_DEBUG)
      : allocator_(boost::addressof(allocator))
#else
      : allocator_(allocator)
#endif // defined(_DEBUG)  
      , handler_(handler)
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~custom_alloc_handler()
    {
#if defined(_DEBUG)
      // For the check of usage of asio custom memory allocation.
      allocator_ = 0;
#endif // defined(_DEBUG)              
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
#if defined(_DEBUG)
      return context->allocator_->allocate(size);
#else
      return context->allocator_.allocate(size);
#endif // defined(_DEBUG)  
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, this_type* context)
    {
#if defined(_DEBUG)
      context->allocator_->deallocate(pointer);
#else
      context->allocator_.deallocate(pointer);
#endif // defined(_DEBUG)  
    } 

#if defined(MA_HAS_RVALUE_REFS)
    template <typename Function>
    friend void asio_handler_invoke(Function&& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(std::forward<Function>(function), context->handler_);
    }
#else
    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
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
#if defined(_DEBUG)
    Allocator* allocator_;
#else
    Allocator& allocator_;
#endif // defined(_DEBUG)
    Handler handler_;
  }; //class custom_alloc_handler 

#if defined(MA_HAS_RVALUE_REFS)
  template <typename Allocator, typename Handler>
  inline custom_alloc_handler<Allocator, typename ma::remove_cv_reference<Handler>::type>
  make_custom_alloc_handler(Allocator& allocator, Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    return custom_alloc_handler<Allocator, handler_type>(allocator, std::forward<Handler>(handler));
  }
#else
  template <typename Allocator, typename Handler>
  inline custom_alloc_handler<Allocator, Handler> 
  make_custom_alloc_handler(Allocator& allocator, const Handler& handler)
  {
    return custom_alloc_handler<Allocator, Handler>(allocator, handler);
  }
#endif // defined(MA_HAS_RVALUE_REFS)
  
} //namespace ma

#endif // MA_CUSTOM_ALLOC_HANDLER_HPP