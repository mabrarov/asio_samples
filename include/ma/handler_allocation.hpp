//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOCATION_HPP
#define MA_HANDLER_ALLOCATION_HPP

#include <cstddef>
#include <boost/utility.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/cstdint.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

namespace ma
{  
  template <std::size_t size_, std::size_t alignment_ = std::size_t(-1)>
  class handler_allocator : private boost::noncopyable
  {  
  public:
    handler_allocator()
      : in_use_(false)
    {
    }

    ~handler_allocator()
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
    boost::aligned_storage<size_, alignment_> storage_;    
    bool in_use_;
  }; //class handler_allocator    

  template <typename Allocator, typename Handler>
  class custom_alloc_handler
  {
  private:
    typedef custom_alloc_handler<Allocator, Handler> this_type;
    const this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    custom_alloc_handler(Allocator& allocator, Handler handler)
      : allocator_(allocator)
      , handler_(handler)
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return context->allocator_.allocate(size);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, this_type* context)
    {
      context->allocator_.deallocate(pointer);
    }

    template <typename Function>
    friend void asio_handler_invoke(Function function, this_type* context)
    {
      ma_invoke_helpers::invoke(function, boost::addressof(context->handler_));
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
    Allocator& allocator_;
    Handler handler_;
  }; //class custom_alloc_handler 

  template <typename Context, typename Handler>
  class context_alloc_handler
  {
  private:
    typedef context_alloc_handler<Context, Handler> this_type;
    const this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    context_alloc_handler(Context context, Handler handler)
      : context_(context)
      , handler_(handler)
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_alloc_helpers::allocate(size, boost::addressof(context->context_));
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_alloc_helpers::deallocate(pointer, size, boost::addressof(context->context_));
    }  

    template <typename Function>
    friend void asio_handler_invoke(Function function, this_type* context)
    {
      ma_invoke_helpers::invoke(function, boost::addressof(context->handler_));
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

  template <typename Allocator, typename Handler>
  inline custom_alloc_handler<Allocator, Handler> 
  make_custom_alloc_handler(Allocator& allocator, Handler handler)
  {
    return custom_alloc_handler<Allocator, Handler>(allocator, handler);
  }

  template <typename Context, typename Handler>
  inline context_alloc_handler<Context, Handler> 
  make_context_alloc_handler(Context context, Handler handler)
  {
    return context_alloc_handler<Context, Handler>(context, handler);
  }  

} //namespace ma

#endif // MA_HANDLER_ALLOCATION_HPP