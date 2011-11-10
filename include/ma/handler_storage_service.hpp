//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/throw_exception.hpp>
#include <ma/config.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

/// Exception thrown when handler_storage::post is used with empty 
/// handler_storage.
class bad_handler_call : public std::runtime_error
{
public:
  bad_handler_call() 
    : std::runtime_error("call to empty ma::handler_storage") 
  {
  } 
}; // class bad_handler_call

/// asio::io_service::service implementing handler_storage.
template <typename Arg>
class handler_storage_service : public boost::asio::io_service::service
{
private:
  typedef handler_storage_service<Arg> this_type;
  typedef boost::mutex mutex_type;
  typedef boost::lock_guard<mutex_type> lock_guard;
  
public:
  typedef Arg arg_type;    

private:
  /// Base class to hold up handlers with the specified signature.
  /**
   * handler_base provides type erasure.
   */
  class handler_base
  {
  private:
    typedef handler_base this_type;

  public:
    typedef void (*post_func_type)(handler_base*, const arg_type&);
    typedef void (*destroy_func_type)(handler_base*);
    typedef void* (*target_func_type)(handler_base*);

    handler_base(post_func_type post_func, destroy_func_type destroy_func,
        target_func_type target_func)
      : post_func_(post_func)
      , destroy_func_(destroy_func)        
      , target_func_(target_func)
    {
    }

    void post(const arg_type& arg)
    {
      post_func_(this, arg);
    }      

    void destroy()
    {
      destroy_func_(this);
    }

    void* target()
    {
      return target_func_(this);
    }

  protected:
    ~handler_base()
    {
    }

  private:        
    post_func_type    post_func_;
    destroy_func_type destroy_func_; 
    target_func_type  target_func_;
  }; // class handler_base    
      
  /// Wrapper class to hold up handlers with the specified signature.
  template <typename Handler>
  class handler_wrapper : public handler_base
  {
  private:
    typedef handler_wrapper<Handler> this_type;

  public:

#if defined(MA_HAS_RVALUE_REFS)

    template <typename H>
    handler_wrapper(boost::asio::io_service& io_service, H&& handler)
      : handler_base(&this_type::do_post, &this_type::do_destroy, 
            &this_type::do_target)
      , io_service_(io_service)
      , work_(io_service)
      , handler_(std::forward<H>(handler))
    {
    }
    
#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

    handler_wrapper(this_type&& other)
      : handler_base(std::move(other))
      , io_service_(other.io_service_)
      , work_(std::move(other.work_))
      , handler_(std::move(other.handler_))
    {
    }

    handler_wrapper(const this_type& other)
      : handler_base(other)
      , io_service_(other.io_service_)
      , work_(other.work_)
      , handler_(other.handler_)
    {
    }

#endif

#else // defined(MA_HAS_RVALUE_REFS)

    handler_wrapper(boost::asio::io_service& io_service, 
        const Handler& handler)
      : handler_base(&this_type::do_post, &this_type::do_destroy, 
            &this_type::do_target)
      , io_service_(io_service)
      , work_(io_service)
      , handler_(handler)
    {
    }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
    ~handler_wrapper()
    {
    }
#endif

    static void do_post(handler_base* base, const arg_type& arg)
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
#endif
      // Change the handler which will be used for wrapper's memory deallocation
      ptr.set_alloc_context(handler);
      // Make copies of other data placed at wrapper object      
      // These copies will be used after the wrapper object destruction 
      // and deallocation of its memory
      boost::asio::io_service& io_service(this_ptr->io_service_);
      boost::asio::io_service::work work(this_ptr->work_);
      (void) work;
      // Destroy wrapper object and deallocate its memory 
      // through the local copy of handler
      ptr.reset();
      // Post the copy of handler's local copy to io_service
#if defined(MA_HAS_RVALUE_REFS)
      io_service.post(detail::bind_handler(std::move(handler), arg));
#else
      io_service.post(detail::bind_handler(handler, arg));
#endif
    }

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
#endif
      // Change the handler which will be used 
      // for wrapper's memory deallocation
      ptr.set_alloc_context(handler);   
      // Destroy wrapper object and deallocate its memory 
      // throw the local copy of handler
      ptr.reset();
    }

    static void* do_target(handler_base* base)
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
    implementation_type()
      : prev_(0)
      , next_(0)
      , handler_(0)
    {
    }

#if !defined(NDEBUG)
    ~implementation_type()
    {
      BOOST_ASSERT(!handler_ && !next_ && !prev_);
    }
#endif

  private:
    friend class handler_storage_service<arg_type>;
    // Pointers to previous and next implementations in a double-linked 
    // intrusive list of implementations.
    implementation_type* prev_;
    implementation_type* next_;
    // Pointer to the stored handler otherwise null pointer.
    handler_base* handler_;
  }; // class implementation_type

private:
  /// Double-linked intrusive list of implementations.
  class impl_list : private boost::noncopyable
  {
  public:
    /// Never throws
    impl_list()
      : front_(0)
    {
    }
    
#if !defined(NDEBUG)
    ~impl_list()
    {
      BOOST_ASSERT(!front_);
    }
#endif

    implementation_type* front() const
    {
      return front_;
    }
    
    /// Never throws
    void push_front(implementation_type& impl)
    {
      BOOST_ASSERT(!impl.next_ && !impl.prev_);

      impl.next_ = front_;      
      if (front_)
      {
        front_->prev_ = &impl;
      }
      front_ = &impl;
    }    
        
    /// Never throws
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

      BOOST_ASSERT(!impl.next_ && !impl.prev_);
    }

    /// Never throws
    void pop_front()
    {
      BOOST_ASSERT(front_);

      implementation_type& impl = *front_;
      front_ = impl.next_;            
      if (front_)
      {
        front_->prev_= 0;
      }
      impl.next_ = impl.prev_ = 0;

      BOOST_ASSERT(!impl.next_ && !impl.prev_);
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

  void construct(implementation_type& impl)
  {
    // Add implementation to the list of active implementations.
    lock_guard lock(impl_list_mutex_);
    impl_list_.push_front(impl);
  }

  void move_construct(implementation_type& impl, 
      implementation_type& other_impl)
  {
    construct(impl);
    // Move ownership of the stored handler
    impl.handler_ = other_impl.handler_;
    other_impl.handler_ = 0;
  }
  
  void destroy(implementation_type& impl)
  {    
    // Remove implementation from the list of active implementations.
    {
      lock_guard lock(impl_list_mutex_);
      impl_list_.erase(impl);
    }    
    // Destroy stored handler if it exists.
    reset(impl);
  }

  /// Can throw if destructor of user supplied handler can throw
  void reset(implementation_type& impl)
  {
    // Destroy stored handler if it exists.
    if (handler_base* handler = impl.handler_)
    {
      impl.handler_ = 0;
      handler->destroy();
    }
  }

  template <typename Handler>
  void reset(implementation_type& impl, Handler handler)
  {
    // If service is (was) in shutdown state then it can't store handler.
    if (!shutdown_done_)
    {      
      typedef handler_wrapper<Handler> value_type;
      typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;
      // Allocate raw memory for storing the handler
      detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
      // Create wrapped handler at allocated memory and 
      // move ownership of allocated memory to ptr
#if defined(MA_HAS_RVALUE_REFS)
      detail::handler_ptr<alloc_traits> ptr(raw_ptr, 
          boost::ref(this->get_io_service()), std::move(handler));
#else
      detail::handler_ptr<alloc_traits> ptr(raw_ptr, 
          boost::ref(this->get_io_service()), handler);
#endif
      // Copy current handler
      handler_base* old_handler = impl.handler_;
      // Move ownership of already created wrapped handler
      // (and allocated memory) to the impl
      impl.handler_ = ptr.release();
      // Destroy previosly stored handler
      if (old_handler)
      {
        old_handler->destroy();
      }
    }
  }

  void post(implementation_type& impl, const arg_type& arg)
  {
    if (handler_base* handler = impl.handler_)
    {
      impl.handler_ = 0;
      handler->post(arg);
    }
    else
    {
      boost::throw_exception(bad_handler_call());
    }
  }

  void* target(const implementation_type& impl) const
  {
    if (impl.handler_)
    {
      return impl.handler_->target();        
    }
    return 0;
  }

  bool empty(const implementation_type& impl) const
  {
    return 0 == impl.handler_;
  }

  bool has_target(const implementation_type& impl) const
  {
    return 0 != impl.handler_;
  }

protected:
  virtual ~handler_storage_service()
  {
  }  

private:
  virtual void shutdown_service()
  {
    // Restrict usage of service.
    shutdown_done_ = true;
    // Clear all still active implementations.
    bool done = false;    
    while (!done)
    {
      // Pop front implementation.
      implementation_type* impl = 0;
      {
        lock_guard lock(impl_list_mutex_);        
        impl = impl_list_.front();
        if (impl)
        {
          impl_list_.pop_front();
        }
      }      
      if (impl)
      {
        // Clear popped implementation.
        reset(*impl);
      }
      else
      {
        // The end of list.
        done = true;
      }
    }
  }

  // Guard for the impl_list_
  mutex_type impl_list_mutex_;
  // Double-linked intrusive list of active (constructed but still not
  // destructed) implementations.
  impl_list impl_list_;
  // Shutdown state flag.
  bool shutdown_done_;
}; // class handler_storage_service

template <typename Arg>
boost::asio::io_service::id handler_storage_service<Arg>::id;
  
} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP
