//
// Copyright (c) 2008-2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOCATION_HPP
#define MA_HANDLER_ALLOCATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

#include <boost/utility.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/scoped_array.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

namespace ma
{  
  template <std::size_t alloc_size>
  class in_place_handler_allocator : private boost::noncopyable
  {  
  public:
    in_place_handler_allocator()
      : in_use_(false)
    {
    }

    ~in_place_handler_allocator()
    {
    }

    void* allocate(std::size_t size)
    {
      if (!in_use_ && size <= storage_.size)
      {
        in_use_ = true;
        return storage_.address();
      }      
      return ::operator new(size);      
    }

    void deallocate(void* pointer)
    {
      if (pointer == storage_.address())
      {        
        in_use_ = false;
        return;
      }      
      ::operator delete(pointer);      
    }

  private:    
    boost::aligned_storage<alloc_size> storage_;    
    bool in_use_;
  }; //class in_place_handler_allocator
  
  class in_heap_handler_allocator : private boost::noncopyable
  {  
  private:
    typedef char byte_type;

    static byte_type* allocate_storage(std::size_t size)
    {      
      std::size_t alloc_size = size;      
      return new byte_type[alloc_size];      
    }

    bool storage_initialized() const
    {
      return 0 != storage_.get();
    }

    byte_type* retrieve_aligned_address()
    {
      if (!storage_.get())
      {
        storage_.reset(allocate_storage(size_));
      }      
      return storage_.get();
    }

  public:
    BOOST_STATIC_CONSTANT(std::size_t, default_size = sizeof(std::size_t) * 64);        

    in_heap_handler_allocator(std::size_t size = default_size, bool lazy = false)
      : storage_(lazy ? 0 : allocate_storage(size))      
      , size_(size)      
      , in_use_(false)
    {      
    }

    ~in_heap_handler_allocator()
    {
    }

    void* allocate(std::size_t size)
    {
      if (!in_use_ && size <= size_)
      {        
        in_use_ = true;
        return retrieve_aligned_address();
      }      
      return ::operator new(size);      
    }

    void deallocate(void* pointer)
    {
      if (storage_initialized())
      {
        if (pointer == retrieve_aligned_address())
        {
          in_use_ = false;
          return;
        }
      }      
      ::operator delete(pointer);      
    }    

  private:    
    boost::scoped_array<byte_type> storage_;    
    std::size_t size_;    
    bool in_use_;
  }; //class in_heap_handler_allocator

  template <typename Allocator, typename Handler>
  class custom_alloc_handler
  {
  private:
    typedef custom_alloc_handler<Allocator, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)
    custom_alloc_handler(Allocator& allocator, Handler&& handler)
#else
    custom_alloc_handler(Allocator& allocator, const Handler& handler)
#endif // defined(MA_HAS_RVALUE_REFS)
#if defined(_DEBUG)
      : allocator_(boost::addressof(allocator))
#else
      : allocator_(allocator)
#endif // defined(_DEBUG)  
#if defined(MA_HAS_RVALUE_REFS)
      , handler_(std::forward<Handler>(handler))
#else
      , handler_(handler)
#endif // defined(MA_HAS_RVALUE_REFS)
    {
    }

    ~custom_alloc_handler()
    {
#if defined(_DEBUG)
      // For the check of usage of asio custom memory allocation.
      allocator_ = 0;
#endif // defined(_DEBUG)              
    }    

#if defined(MA_HAS_RVALUE_REFS)
    custom_alloc_handler(this_type&& other)
      : allocator_(other.allocator_)
      , handler_(std::move(other.handler_))
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

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

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
    }  

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

  template <typename Allocator, typename Handler>
#if defined(MA_HAS_RVALUE_REFS)
  inline custom_alloc_handler<Allocator, typename std::identity<Handler>::type> 
  make_custom_alloc_handler(Allocator& allocator, Handler&& handler)
#else
  inline custom_alloc_handler<Allocator, Handler> 
  make_custom_alloc_handler(Allocator& allocator, const Handler& handler)
#endif // defined(MA_HAS_RVALUE_REFS)  
  {
#if defined(MA_HAS_RVALUE_REFS)
    typedef typename std::identity<Handler>::type handler_type;
    return custom_alloc_handler<Allocator, handler_type>(allocator, std::forward<Handler>(handler));
#else  
    return custom_alloc_handler<Allocator, Handler>(allocator, handler);
#endif // defined(MA_HAS_RVALUE_REFS)      
  }    

  template <typename Context, typename Handler>
  class context_alloc_handler
  {
  private:
    typedef context_alloc_handler<Context, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    context_alloc_handler(const Context& context, const Handler& handler)
      : context_(context)
      , handler_(handler)
    {
    }

#if defined(MA_HAS_RVALUE_REFS)
    context_alloc_handler(this_type&& other)
      : context_(std::move(other.context_))
      , handler_(std::move(other.handler_))
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~context_alloc_handler()
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->context_);
    }  

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
    } 

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
    Context context_;
    Handler handler_;
  }; //class context_alloc_handler

  template <typename Context, typename Handler>
  inline context_alloc_handler<Context, Handler>
  make_context_alloc_handler(const Context& context, const Handler& handler)
  {
    return context_alloc_handler<Context, Handler>(context, handler);
  }  
  
  template <typename Context, typename Handler>
  class context_alloc_handler2
  {
  private:
    typedef context_alloc_handler2<Context, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    context_alloc_handler2(const Context& context, const Handler& handler)
      : context_(context)
      , handler_(handler)
    {
    }

#if defined(MA_HAS_RVALUE_REFS)
    context_alloc_handler2(this_type&& other)
      : context_(std::move(other.context_))
      , handler_(std::move(other.handler_))
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~context_alloc_handler2()
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->context_);
    }  

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
    } 
    
    void operator()()
    {
      handler_(context_);
    }

    template <typename Arg1>
    void operator()(const Arg1& arg1)
    {
      handler_(context_, arg1);
    }

    template <typename Arg1, typename Arg2>
    void operator()(const Arg1& arg1, const Arg2& arg2)
    {
      handler_(context_, arg1, arg2);
    }

    template <typename Arg1, typename Arg2, typename Arg3>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
    {
      handler_(context_, arg1, arg2, arg3);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    {
      handler_(context_, arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    {
      handler_(context_, arg1, arg2, arg3, arg4, arg5);
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
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) const
    {
      handler_(context_, arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) const
    {
      handler_(context_, arg1, arg2, arg3, arg4, arg5);
    }

  private:
    Context context_;
    Handler handler_;
  }; //class context_alloc_handler2  

  template <typename Context, typename Handler>
  inline context_alloc_handler2<Context, Handler>
  make_context_alloc_handler2(const Context& context, const Handler& handler)
  {
    return context_alloc_handler2<Context, Handler>(context, handler);
  }  

  template <typename Context, typename Handler>
  class context_wrapped_handler
  {
  private:
    typedef context_wrapped_handler<Context, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    context_wrapped_handler(const Context& context, const Handler& handler)
      : context_(context)
      , handler_(handler)
    {
    }

#if defined(MA_HAS_RVALUE_REFS)
    context_wrapped_handler(this_type&& other)
      : context_(std::move(other.context_))
      , handler_(std::move(other.handler_))
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~context_wrapped_handler()
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->context_);
    }  

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->context_);
    } 

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
    Context context_;
    Handler handler_;
  }; //class context_wrapped_handler

  template <typename Context, typename Handler>
  inline context_wrapped_handler<Context, Handler> 
  make_context_wrapped_handler(const Context& context, const Handler& handler)
  {
    return context_wrapped_handler<Context, Handler>(context, handler);
  }  
  
  template <typename Context, typename Handler>
  class context_wrapped_handler2
  {
  private:
    typedef context_wrapped_handler2<Context, Handler> this_type;
    this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    context_wrapped_handler2(const Context& context, const Handler& handler)
      : context_(context)
      , handler_(handler)
    {
    }

#if defined(MA_HAS_RVALUE_REFS)
    context_wrapped_handler2(this_type&& other)
      : context_(std::move(other.context_))
      , handler_(std::move(other.handler_))
    {
    }
#endif // defined(MA_HAS_RVALUE_REFS)

    ~context_wrapped_handler2()
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_asio_handler_alloc_helpers::allocate(size, context->context_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->context_);
    }  

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->context_);
    } 
    
    void operator()()
    {
      handler_(context_);
    }

    template <typename Arg1>
    void operator()(const Arg1& arg1)
    {
      handler_(context_, arg1);
    }

    template <typename Arg1, typename Arg2>
    void operator()(const Arg1& arg1, const Arg2& arg2)
    {
      handler_(context_, arg1, arg2);
    }

    template <typename Arg1, typename Arg2, typename Arg3>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
    {
      handler_(context_, arg1, arg2, arg3);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    {
      handler_(context_, arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    {
      handler_(context_, arg1, arg2, arg3, arg4, arg5);
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
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4) const
    {
      handler_(context_, arg1, arg2, arg3, arg4);
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5) const
    {
      handler_(context_, arg1, arg2, arg3, arg4, arg5);
    }

  private:
    Context context_;
    Handler handler_;
  }; //class context_wrapped_handler2
        
  template <typename Context, typename Handler>
  inline context_wrapped_handler2<Context, Handler> 
  make_context_wrapped_handler2(const Context& context, const Handler& handler)
  {
    return context_wrapped_handler2<Context, Handler>(context, handler);
  }  

} //namespace ma

#endif // MA_HANDLER_ALLOCATION_HPP