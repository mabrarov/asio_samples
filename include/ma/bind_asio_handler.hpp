//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_BIND_ASIO_HANDLER_HPP
#define MA_BIND_ASIO_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

namespace ma
{
  namespace detail 
  {
    template <typename Handler, typename Arg1>
    class binder1
    {
    private:
      typedef binder1<Handler, Arg1> this_type;
      this_type& operator=(const this_type&);

    public:
      typedef void result_type;

      binder1(const Handler& handler, const Arg1& arg1)
        : handler_(handler)
        , arg1_(arg1)
      {
      }

  #if defined(MA_HAS_RVALUE_REFS)
      binder1(this_type&& other)
        : handler_(std::move(other.handler_))
        , arg1_(std::move(other.arg1_))
      {
      }
  #endif // defined(MA_HAS_RVALUE_REFS)

      ~binder1()
      {
      }

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
        return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
      }

      friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
      {
        ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
      }

      template <typename Function>
      friend void asio_handler_invoke(const Function& function, this_type* context)
      {
        ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
      }

    private:      
      Handler handler_;
      const Arg1 arg1_;
    }; // class binder1  

    template <typename Handler, typename Arg1>
    inline binder1<Handler, Arg1> 
    bind_handler(const Handler& handler, const Arg1& arg1)
    {
      return binder1<Handler, Arg1>(handler, arg1);
    } // bind_handler

    template <typename Handler, typename Arg1, typename Arg2>
    class binder2
    {
    private:
      typedef binder2<Handler, Arg1, Arg2> this_type;
      this_type& operator=(const this_type&);

    public:
      typedef void result_type;

      binder2(const Handler& handler, const Arg1& arg1, const Arg2& arg2)
        : handler_(handler)
        , arg1_(arg1)
        , arg2_(arg2)
      {
      }

  #if defined(MA_HAS_RVALUE_REFS)
      binder2(this_type&& other)
        : handler_(std::move(other.handler_))
        , arg1_(std::move(other.arg1_))
        , arg2_(std::move(other.arg2_))
      {
      }
  #endif // defined(MA_HAS_RVALUE_REFS)

      ~binder2()
      {
      }

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
        return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
      }

      friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
      {
        ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
      }

      template <typename Function>
      friend void asio_handler_invoke(const Function& function, this_type* context)
      {
        ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
      }

    private:      
      Handler handler_;
      const Arg1 arg1_;
      const Arg2 arg2_;      
    }; // class binder2    

    template <typename Handler, typename Arg1, typename Arg2>
    inline binder2<Handler, Arg1, Arg2>
    bind_handler(const Handler& handler, const Arg1& arg1, const Arg2& arg2)
    {
      return binder2<Handler, Arg1, Arg2>(handler, arg1, arg2);
    } // bind_handler

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
    class binder3
    {
    private:
      typedef binder3<Handler, Arg1, Arg2, Arg3> this_type;
      this_type& operator=(const this_type&);

    public:
      typedef void result_type;

      binder3(const Handler& handler, const Arg1& arg1, const Arg2& arg2, 
        const Arg3& arg3)
        : handler_(handler)
        , arg1_(arg1)
        , arg2_(arg2)
        , arg3_(arg3)
      {
      }

  #if defined(MA_HAS_RVALUE_REFS)
      binder3(this_type&& other)
        : handler_(std::move(other.handler_))
        , arg1_(std::move(other.arg1_))
        , arg2_(std::move(other.arg2_))
        , arg3_(std::move(other.arg3_))
      {
      }
  #endif // defined(MA_HAS_RVALUE_REFS)

      ~binder3()
      {
      }

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
        return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
      }

      friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
      {
        ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
      }

      template <typename Function>
      friend void asio_handler_invoke(const Function& function, this_type* context)
      {
        ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
      }

    private:      
      Handler handler_;
      const Arg1 arg1_;
      const Arg2 arg2_;
      const Arg3 arg3_;
    }; // class binder3    

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
    inline binder3<Handler, Arg1, Arg2, Arg3>
    bind_handler(const Handler& handler, const Arg1& arg1, const Arg2& arg2, 
      const Arg3& arg3)
    {
      return binder3<Handler, Arg1, Arg2, Arg3>(handler, arg1, arg2, arg3);
    } // bind_handler

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3, 
      typename Arg4>
    class binder4
    {
    private:
      typedef binder4<Handler, Arg1, Arg2, Arg3, Arg4> this_type;
      this_type& operator=(const this_type&);

    public:
      binder4(const Handler& handler, const Arg1& arg1, const Arg2& arg2,
        const Arg3& arg3, const Arg4& arg4)
        : handler_(handler)
        , arg1_(arg1)
        , arg2_(arg2)
        , arg3_(arg3)
        , arg4_(arg4)
      {
      }

  #if defined(MA_HAS_RVALUE_REFS)
      binder4(this_type&& other)
        : handler_(std::move(other.handler_))
        , arg1_(std::move(other.arg1_))
        , arg2_(std::move(other.arg2_))
        , arg3_(std::move(other.arg3_))
        , arg4_(std::move(other.arg4_))
      {
      }
  #endif // defined(MA_HAS_RVALUE_REFS)

      ~binder4()
      {
      }

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
        return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
      }

      friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
      {
        ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
      }

      template <typename Function>
      friend void asio_handler_invoke(const Function& function, this_type* context)
      {
        ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
      }

    private:      
      Handler handler_;
      const Arg1 arg1_;
      const Arg2 arg2_;
      const Arg3 arg3_;
      const Arg4 arg4_;      
    }; // class binder4    

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3, 
      typename Arg4>
    inline binder4<Handler, Arg1, Arg2, Arg3, Arg4>
    bind_handler(const Handler& handler, const Arg1& arg1, const Arg2& arg2,
      const Arg3& arg3, const Arg4& arg4)
    {
      return binder4<Handler, Arg1, Arg2, Arg3, Arg4>(handler, 
        arg1, arg2, arg3, arg4);
    } // bind_handler

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3,
      typename Arg4, typename Arg5>
    class binder5
    {
    private:
      typedef binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5> this_type;
      this_type& operator=(const this_type&);

    public:
      binder5(const Handler& handler, const Arg1& arg1, const Arg2& arg2, 
        const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
        : handler_(handler)
        , arg1_(arg1)
        , arg2_(arg2)
        , arg3_(arg3)
        , arg4_(arg4)
        , arg5_(arg5)
      {
      }

  #if defined(MA_HAS_RVALUE_REFS)
      binder5(this_type&& other)
        : handler_(std::move(other.handler_))
        , arg1_(std::move(other.arg1_))
        , arg2_(std::move(other.arg2_))
        , arg3_(std::move(other.arg3_))
        , arg4_(std::move(other.arg4_))
        , arg5_(std::move(other.arg5_))
      {
      }
  #endif // defined(MA_HAS_RVALUE_REFS)

      ~binder5()
      {
      }

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
        return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
      }

      friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
      {
        ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
      }

      template <typename Function>
      friend void asio_handler_invoke(const Function& function, this_type* context)
      {
        ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
      }

    private:      
      Handler handler_;
      const Arg1 arg1_;
      const Arg2 arg2_;
      const Arg3 arg3_;
      const Arg4 arg4_;
      const Arg5 arg5_;      
    }; // class binder5 

    template <typename Handler, typename Arg1, typename Arg2, typename Arg3, 
      typename Arg4, typename Arg5>
    inline binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>
    bind_handler(const Handler& handler, const Arg1& arg1, const Arg2& arg2, 
      const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    {
      return binder5<Handler, Arg1, Arg2, Arg3, Arg4, Arg5>(handler, 
        arg1, arg2, arg3, arg4, arg5);
    } // bind_handler

  } // namespace detail  
} // namespace ma

#endif // MA_BIND_ASIO_HANDLER_HPP
