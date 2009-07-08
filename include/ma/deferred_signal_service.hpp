//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DEFERRED_SIGNAL_SERVICE_HPP
#define MA_DEFERRED_SIGNAL_SERVICE_HPP

#include <cstddef>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/call_traits.hpp>

namespace ma
{
  template <typename Arg>
  class deferred_signal_service : public boost::asio::io_service::service
  {
  private:
    typedef deferred_signal_service<Arg> this_type;    
    typedef boost::mutex mutex_type;

  public:
    typedef Arg argument_type;

  private:    
    typedef typename boost::call_traits<argument_type>::param_type arg_param_type;

    class handler_base
    {
    public:
      typedef void (*invoke_func_type)(handler_base*, arg_param_type);
      typedef void (*destroy_func_type)(handler_base*);      
      
      handler_base* next_;

      explicit handler_base(invoke_func_type invoke_func, 
        destroy_func_type destroy_func, arg_param_type cancel_arg)
        : next_(0)
        , invoke_func_(invoke_func)
        , destroy_func_(destroy_func)
        , cancel_arg_(cancel_arg)
      {
      }

      void invoke(arg_param_type arg)
      {
        invoke_func_(this, arg);
      }

      void cancel()
      {
        // Make a copy of the cancel_arg_ so that the memory (*this) 
        // can be safely deallocated before invoke_func_ return
        argument_type cancel_arg(cancel_arg_);
        invoke_func_(this, cancel_arg);
      }

      void destroy()
      {
        destroy_func_(this);
      }

    protected:
      ~handler_base()
      {
      }

    private:        
      invoke_func_type invoke_func_;
      destroy_func_type destroy_func_;
      argument_type cancel_arg_;
    }; // class handler_base    
      
    template <typename Handler>
    class handler_wrapper : public handler_base
    {
    private:
      typedef handler_wrapper<Handler> this_type;
      const this_type& operator=(const this_type&);    

    public:
      explicit handler_wrapper(boost::asio::io_service& io_service, 
        arg_param_type cancel_arg, Handler handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy, cancel_arg)
        , io_service_(io_service)
        , work_(io_service)        
        , handler_(handler)
      {
      }

      ~handler_wrapper()
      {
      }

      static void do_invoke(handler_base* base, arg_param_type arg)
      {
        // Take ownership of the handler object.          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler and other needed object's data 
        // so that the memory can be deallocated before the upcall is made.
        boost::asio::io_service& io_service(h->io_service_);
        boost::asio::io_service::work work(h->work_);                
        (void) work; // Optimization avoid
        Handler handler(h->handler_);                  

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        io_service.post        
        (
          boost::asio::detail::bind_handler
          (
            handler, arg
          )          
        );
      }

      static void do_destroy(handler_base* base)
      {          
        // Take ownership of the handler object.          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler and other needed object's data 
        // so that the memory can be deallocated before the upcall is made.        
        boost::asio::io_service::work work(h->work_);                
        (void) work; // Optimization avoid
        Handler handler(h->handler_); 
        (void) handler; // Optimization avoid

        // Free the memory associated with the handler.
        ptr.reset();
      }

    private:
      boost::asio::io_service& io_service_;
      boost::asio::io_service::work work_;
      Handler handler_;      
    }; // class handler_wrapper

    class handler_queue : private boost::noncopyable
    {
    public:
      handler_queue() // nothrow
        : front_(0)
      {
      }

      handler_queue(handler_base* front) // nothrow
        : front_(front)
      {
      }
      
      void swap(handler_queue& other) // nothrow
      {
        handler_base* tmp(other.front_);
        other.front_ = front_;
        front_ = tmp;
      }

      handler_base* release() // nothrow
      {
        handler_base* tmp = front_;
        front_ = 0;
        return tmp;
      }

      // Whether the queue is empty.
      bool empty() const
      {
        return 0 == front_;
      }

      ~handler_queue() // may throw! (if handler's copy constructor may throw)
      {
        while (front_)
        {
          handler_queue tmp(front_->next_);          
          front_->destroy();
          front_ = tmp.release();
        }        
      }

    private:
      handler_base* front_;
    }; // class handler_queue

  public:
    static boost::asio::io_service::id id;

    class implementation_type : private boost::noncopyable
    { 
    public:
      explicit implementation_type()
        : prev_(0)
        , next_(0)
      {
      }

    private:
      friend class deferred_signal_service<argument_type>;
      implementation_type* prev_;
      implementation_type* next_;      
      handler_queue handler_queue_;
    };

    explicit deferred_signal_service(boost::asio::io_service& io_service)
      : boost::asio::io_service::service(io_service)
      , impl_list_(0)
      , shutdown_done_(false)
    {
    }

    ~deferred_signal_service()
    {
    }

    void shutdown_service()
    {   
      shutdown_done_ = true;
      while (impl_list_)
      {
        // Take the ownership of the handlers' queue (nothrow)
        handler_queue local_queue(impl_list_->handler_queue_.release());

        // Exclude front implementation from the linked list of implementations
        implementation_type* tmp = impl_list_->next_;        
        impl_list_->prev_ = 0;
        impl_list_->next_ = 0;
        impl_list_ = tmp;        
        if (impl_list_)
        {
          impl_list_->prev_ = 0;
        }        
      }
    }    

    void construct(implementation_type& impl)
    {
      if (!shutdown_done_)
      {
        // Insert impl into the linked list of implementations.
        mutex_type::scoped_lock lock(mutex_);
        impl.next_ = impl_list_;
        impl.prev_ = 0;
        if (impl_list_)
        {
          impl_list_->prev_ = &impl;
        }
        impl_list_ = &impl;
      }
    }

    void destroy(implementation_type& impl)
    {
      if (!shutdown_done_)
      {
        // Take the ownership (nothrow)
        handler_queue local_queue(impl.handler_queue_.release());

        // Exclude impl from the linked list of implementations.
        mutex_type::scoped_lock lock(mutex_);
        if (impl_list_ == &impl)
        {
          impl_list_ = impl.next_;
        }
        if (impl.prev_)
        {
          impl.prev_->next_ = impl.next_;
        }
        if (impl.next_)
        {
          impl.next_->prev_= impl.prev_;
        }
        lock.unlock();
        impl.prev_ = impl.next_ = 0;        
      }
    }

    template <typename Handler>
    void async_wait(implementation_type& impl, arg_param_type cancel_arg, Handler handler)
    {      
      if (!shutdown_done_)
      {
        typedef handler_wrapper<Handler> value_type;
        typedef boost::asio::detail::handler_alloc_traits<Handler, value_type> alloc_traits;
        // Allocate raw memory for handler
        boost::asio::detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
        // Wrap local handler and copy wrapper into allocated memory
        boost::asio::detail::handler_ptr<alloc_traits> ptr(raw_ptr, 
          this->get_io_service(), cancel_arg, handler);               

        // Take the ownership (nothrow)
        handler_base* handler_ptr = ptr.release();        

        // Take the ownership (nothrow)
        handler_ptr->next_ = impl.handler_queue_.release();

        // Take the ownership (nothrow)
        handler_queue(handler_ptr).swap(impl.handler_queue_);
      }
    }

    std::size_t fire(implementation_type& impl, arg_param_type arg)
    {      
      // Take the ownership
      handler_queue local_queue(impl.handler_queue_.release());      

      std::size_t num_handlers = 0;      
      while (!local_queue.empty())
      { 
        handler_base* handler_ptr(local_queue.release());
        handler_queue guard(handler_ptr->next_);
        handler_ptr->invoke(arg);
        ++num_handlers;
        local_queue.swap(guard);
      }      
      return num_handlers;
    }

    std::size_t cancel(implementation_type& impl)
    {      
      // Take the ownership
      handler_queue local_queue(impl.handler_queue_.release());      

      std::size_t num_handlers = 0;      
      while (!local_queue.empty())
      { 
        handler_base* handler_ptr(local_queue.release());
        handler_queue guard(handler_ptr->next_);
        handler_ptr->cancel();
        ++num_handlers;
        local_queue.swap(guard);
      }      
      return num_handlers;
    }    

  private:
    mutex_type mutex_;
    implementation_type* impl_list_;
    bool shutdown_done_;
  }; // class deferred_signal_service

  template <typename Arg>
  boost::asio::io_service::id deferred_signal_service<Arg>::id;

} // namespace ma

#endif // MA_DEFERRED_SIGNAL_HPP