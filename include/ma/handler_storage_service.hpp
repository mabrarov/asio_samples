//
// Copyright (c) 2008-2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_SERVICE_HPP
#define MA_HANDLER_STORAGE_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <stdexcept>
#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>
#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/call_traits.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/bind_asio_handler.hpp>

namespace ma
{  
  class bad_handler_call : public std::runtime_error
  {
  public:
    explicit bad_handler_call() 
      : std::runtime_error("call to empty ma::handler_storage") 
    {
    } 
  }; // class bad_handler_call

  template <typename Arg>
  class handler_storage_service : public boost::asio::io_service::service
  {
  private:
    typedef handler_storage_service<Arg> this_type;    
    typedef boost::mutex mutex_type;

  public:
    typedef Arg argument_type;    

  private:    
    typedef typename boost::call_traits<argument_type>::param_type arg_param_type;    

    class handler_base
    {
    private:
      typedef handler_base this_type;
      this_type& operator=(const this_type&);

    public:
      typedef void (*invoke_func_type)(handler_base*, arg_param_type);
      typedef void (*destroy_func_type)(handler_base*);
      typedef void* (*data_func_type)(handler_base*);

      explicit handler_base(invoke_func_type invoke_func, 
        destroy_func_type destroy_func, data_func_type data_func)
        : invoke_func_(invoke_func)
        , destroy_func_(destroy_func)        
        , data_func_(data_func)
      {
      }

      void invoke(arg_param_type arg)
      {
        invoke_func_(this, arg);
      }      

      void destroy()
      {
        destroy_func_(this);
      }

      void* data()
      {
        return data_func_(this);
      }

    protected:
      ~handler_base()
      {
      }

    private:        
      invoke_func_type invoke_func_;
      destroy_func_type destroy_func_; 
      data_func_type data_func_;
    }; // class handler_base    
      
    template <typename Handler>
    class handler_wrapper : public handler_base
    {
    private:
      typedef handler_wrapper<Handler> this_type;
      this_type& operator=(const this_type&);    

    public:
      explicit handler_wrapper(boost::asio::io_service& io_service, const Handler& handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy, &this_type::do_data)
        , io_service_(io_service)
        , work_(io_service)
        , handler_(handler)
      {
      }      

#if defined(MA_HAS_RVALUE_REFS)
      // Move constructor
      handler_wrapper(this_type&& other)
        : handler_base(std::move(other))
        , io_service_(other.io_service_)
        , work_(std::move(other.work_))
        , handler_(std::move(other.handler_))
      {
      }
#endif // defined(MA_HAS_RVALUE_REFS)

      ~handler_wrapper()
      {
      }

      static void do_invoke(handler_base* base, arg_param_type arg)
      {        
        this_type* this_ptr = static_cast<this_type*>(base);
        // Take ownership of the wrapper object
        // The deallocation of wrapper object will be done 
        // throw the handler stored in wrapper
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(this_ptr->handler_, this_ptr);          
        // Make a local copy of handler stored at wrapper object
        // This local copy will be used for wrapper's memory deallocation later
#if defined(MA_HAS_RVALUE_REFS)
        Handler handler(std::move(this_ptr->handler_));
#else
        Handler handler(this_ptr->handler_);
#endif // defined(MA_HAS_RVALUE_REFS)
        // Change the handler which will be used for wrapper's memory deallocation
        ptr.set_alloc_context(handler);
        // Make copies of other data placed at wrapper object      
        // These copies will be used after the wrapper object destruction 
        // and deallocation of its memory
        boost::asio::io_service& io_service(this_ptr->io_service_);
        boost::asio::io_service::work work(this_ptr->work_);
        // A dummy vs optimization.
        (void) work;        
        // Destroy wrapper object and deallocate its memory 
        // throw the local copy of handler
        ptr.reset();          
        // Post the copy of handler's local copy to io_service
#if defined(MA_HAS_RVALUE_REFS)
        io_service.post(detail::bind_handler(std::move(handler), arg));
#else
        io_service.post(detail::bind_handler(handler, arg));
#endif // defined(MA_HAS_RVALUE_REFS)        
      }  // do_invoke

      static void do_destroy(handler_base* base)
      {          
        this_type* this_ptr = static_cast<this_type*>(base);
        // Take ownership of the wrapper object
        // The deallocation of wrapper object will be done 
        // throw the handler stored in wrapper
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(this_ptr->handler_, this_ptr);          
        // Make a local copy of handler stored at wrapper object
        // This local copy will be used for wrapper's memory deallocation later
#if defined(MA_HAS_RVALUE_REFS)
        Handler handler(std::move(this_ptr->handler_));
#else
        Handler handler(this_ptr->handler_);
#endif // defined(MA_HAS_RVALUE_REFS)
        // Change the handler which will be used for wrapper's memory deallocation
        ptr.set_alloc_context(handler);   
        // Destroy wrapper object and deallocate its memory 
        // throw the local copy of handler
        ptr.reset();
      } // do_destroy

      static void* do_data(handler_base* base)
      {          
        this_type* this_ptr = static_cast<this_type*>(base);
        return boost::addressof(this_ptr->handler_);
      }

    private:
      boost::asio::io_service& io_service_;
      boost::asio::io_service::work work_;
      Handler handler_;      
    }; // class handler_wrapper
    
  public:
    static boost::asio::io_service::id id;

    class implementation_type : private boost::noncopyable
    { 
    public:
      explicit implementation_type()
        : prev_(0)
        , next_(0)
        , handler_ptr_(0)
      {
      }

    private:
      friend class handler_storage_service<argument_type>;
      implementation_type* prev_;
      implementation_type* next_;      
      handler_base* handler_ptr_;
    }; // class implementation_type

  private:
    class impl_list : private boost::noncopyable
    {
    public:
      explicit impl_list()
        : front_(0)
      {
      }

      void push_front(implementation_type& impl)
      {
        BOOST_ASSERT(!impl.next_ && !impl.prev_);
        impl.next_ = front_;
        impl.prev_ = 0;
        if (front_)
        {
          front_->prev_ = &impl;
        }
        front_ = &impl;
      } // push_front

      void erase(implementation_type& impl)
      {
        if (front_ == &impl)
        {
          front_ = impl.next_;
        }
        if (impl.prev_)
        {
          impl.prev_->next_ = impl.next_;
        }
        if (impl.next_)
        {
          impl.next_->prev_= impl.prev_;
        }
        impl.next_ = impl.prev_ = 0;
      } // erase

      implementation_type* const front() const
      {
        return front_;
      }

      bool empty() const
      {
        return 0 == front_;
      }

    private:
      implementation_type* front_;
    }; // class impl_list

  public:    
    explicit handler_storage_service(boost::asio::io_service& io_service)
      : boost::asio::io_service::service(io_service)      
      , shutdown_done_(false)
    {
    }

    ~handler_storage_service()
    {
    }

    void shutdown_service()
    {   
      shutdown_done_ = true;
      while (!impl_list_.empty())
      {
        destroy(*impl_list_.front());
      }      
    } // shutdown_service

    void construct(implementation_type& impl)
    {      
      mutex_type::scoped_lock lock(mutex_);
      impl_list_.push_front(impl);     
    } // construct

    void destroy(implementation_type& impl)
    {   
      mutex_type::scoped_lock lock(mutex_);        
      impl_list_.erase(impl);
      lock.unlock();

      handler_base* handler_ptr = impl.handler_ptr_;
      impl.handler_ptr_ = 0;      
      if (handler_ptr)
      {
        handler_ptr->destroy();
      }
    } // destroy

    template <typename Handler>
    void put(implementation_type& impl, Handler handler)
    {
      if (!shutdown_done_)
      {      
        typedef handler_wrapper<Handler> value_type;
        typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;
        // Allocate raw memory for handler
        detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
        // Wrap local handler and copy wrapper into allocated memory
        detail::handler_ptr<alloc_traits> ptr(raw_ptr, 
          boost::ref(this->get_io_service()), handler);
        // Copy current handler ptr
        handler_base* handler_ptr = impl.handler_ptr_;
        // Take the ownership
        impl.handler_ptr_ = ptr.release();
        if (handler_ptr)
        {
          handler_ptr->destroy();
        }
      } // if (!shutdown_done_)
    } // put

    void post(implementation_type& impl, arg_param_type arg) const
    {      
      if (!impl.handler_ptr_)
      {
        boost::throw_exception(bad_handler_call());
      }
      // Take the ownership
      handler_base* handler_ptr = impl.handler_ptr_;
      impl.handler_ptr_ = 0;
      handler_ptr->invoke(arg);
    } // post

    void* target(const implementation_type& impl) const
    {
      if (impl.handler_ptr_)
      {
        return impl.handler_ptr_->data();        
      }
      return 0;
    } // target

    bool empty(const implementation_type& impl) const
    {
      return 0 == impl.handler_ptr_;
    }

    bool has_target(const implementation_type& impl) const
    {
      return 0 != impl.handler_ptr_;
    }

  private:
    mutex_type mutex_;
    impl_list impl_list_;
    bool shutdown_done_;
  }; // class handler_storage_service

  template <typename Arg>
  boost::asio::io_service::id handler_storage_service<Arg>::id;
  
} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP