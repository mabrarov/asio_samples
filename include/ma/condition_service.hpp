//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONDITION_SERVICE_HPP
#define MA_CONDITION_SERVICE_HPP

#include <cstddef>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

namespace ma
{
  template <typename Arg>
  class condition_service : public boost::asio::io_service::service
  {
  private:
    typedef condition_service<Arg> this_type;    
    typedef boost::mutex mutex_type;

  public:
    typedef Arg argument_type;    

  private:    
    class handler_base
    {
    public:
      typedef void (*invoke_func_type)(handler_base*, argument_type);
      typedef void (*destroy_func_type)(handler_base*);      
      
      handler_base* next;

      explicit handler_base(invoke_func_type invoke_func, destroy_func_type destroy_func)
        : next(0)
        , invoke_func_(invoke_func)
        , destroy_func_(destroy_func)
      {
      }

      void invoke(const argument_type& arg)
      {
        invoke_func_(this, arg);
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
    }; // class handler_base    
      
    template <typename Handler>
    class handler_wrapper : public handler_base
    {
    private:
      typedef handler_wrapper<Handler> this_type;
      const this_type& operator=(const this_type&);    

    public:
      explicit handler_wrapper(boost::asio::io_service& io_service, 
        argument_type cancel_arg, Handler handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy)
        , io_service_(io_service)        
        , work_(io_service)
        , cancel_arg_(cancel_arg)
        , handler_(handler)
      {
      }

      static void do_invoke(handler_base* base, argument_type arg)
      {
        // Take ownership of the handler object.          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made.
        boost::asio::io_service& io_service(h->io_service_);
        boost::asio::io_service::work work(h->work_);
        Handler handler(h->handler_);          

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        io_service.post        
        (
          boost::asio::detail::bind_handler
          (
            handler, 
            arg
          )          
        );
      }

      static void do_destroy(handler_base* base)
      {          
        // Take ownership of the handler object.          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made.
        boost::asio::io_service& io_service(h->io_service_);
        boost::asio::io_service::work work(h->work_);
        argument_type arg(h->cancel_arg_);
        Handler handler(h->handler_);          

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        io_service.post        
        (
          boost::asio::detail::bind_handler
          (
            handler, 
            arg
          )          
        );
      }

    private:
      boost::asio::io_service& io_service_;
      boost::asio::io_service::work work_;
      argument_type cancel_arg_;
      Handler handler_;      
    }; // class handler_wrapper    

  public:
    static boost::asio::io_service::id id;

    struct implementation_type
    {    
      explicit implementation_type()
        : prev(0)
        , next(0)
        , handler_count(0)
        , handler_queue(0)
      {
      }

      implementation_type* prev;
      implementation_type* next;
      std::size_t handler_count;
      handler_base* handler_queue;
    };

    explicit condition_service(boost::asio::io_service& io_service)
      : boost::asio::io_service::service(io_service)
      , impl_list_(0)
      , shutdown_done_(false)
    {
    }

    ~condition_service()
    {
    }

    void shutdown_service()
    {   
      shutdown_done_ = true;
      while (impl_list_)
      {
        // Copy implementation
        implementation_type impl(*impl_list_);

        // Clear original implementation 
        impl_list_->prev = 0;
        impl_list_->next = 0;
        impl_list_->handler_count = 0;
        impl_list_->handler_queue = 0;        

        // Iterate to the next implementation
        impl_list_ = impl.next;

        // Destroy all queued handlers
        while (impl.handler_queue)
        {
          handler_base* handler_ptr(impl.handler_queue);
          impl.handler_queue = handler_ptr->next;
          handler_ptr->destroy();
        }
      }
    }    

    void construct(implementation_type& impl)
    {
      if (!shutdown_done_)
      {
        // Insert impl into linked list of all implementations.
        mutex_type::scoped_lock lock(mutex_);
        impl.next = impl_list_;
        impl.prev = 0;
        if (impl_list_)
        {
          impl_list_->prev = &impl;
        }
        impl_list_ = &impl;
      }
    }

    void destroy(implementation_type& impl)
    {
      if (!shutdown_done_)
      {
        // Delete impl from linked list of all implementations.
        mutex_type::scoped_lock lock(mutex_);
        if (impl_list_ == &impl)
        {
          impl_list_ = impl.next;
        }
        if (impl.prev)
        {
          impl.prev->next = impl.next;
        }
        if (impl.next)
        {
          impl.next->prev= impl.prev;
        }
        impl.prev = 0;        
        impl.next = 0;
        while (impl.handler_queue)
        {
          handler_base* handler_ptr(impl.handler_queue);
          impl.handler_queue = handler_ptr->next;
          handler_ptr->destroy();
        }
        impl.handler_count = 0;
      }      
    }

    template <typename Handler>
    void async_wait(implementation_type impl, argument_type cancel_arg, Handler handler)
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

        // Take the ownership
        handler_base* handler_ptr = ptr.release();

        // Push in front of the implementation's handler queue
        if (impl.handler_queue)
        {
          handler_ptr->next = impl.handler_queue;
        }
        impl.handler_queue = handler_ptr;
        ++impl.handler_count;
      }
    }

    std::size_t fire_now(implementation_type& impl, argument_type arg)
    {      
      std::size_t fired_count = 0;
      if (!shutdown_done_)
      {
        while (impl.handler_queue)
        {
          handler_base* handler_ptr(impl.handler_queue);
          impl.handler_queue = handler_ptr->next;
          --impl.handler_count;
          ++fired_count;
          handler_ptr->invoke(arg);        
        }
      }
      return fired_count;
    }

    std::size_t cancel(implementation_type& impl)
    {      
      std::size_t canceled_count = 0;
      if (!shutdown_done_)
      {
        while (impl.handler_queue)
        {
          handler_base* handler_ptr(impl.handler_queue);
          impl.handler_queue = handler_ptr->next;
          --impl.handler_count;
          ++canceled_count;
          handler_ptr->destroy();
        }
      }
      return canceled_count;
    }

    std::size_t get_charge(implementation_type& impl)
    {
      return impl.handler_count;
    }

  private:
    mutex_type mutex_;
    implementation_type* impl_list_;
    bool shutdown_done_;
  }; // class condition_service

  template <typename Arg>
  boost::asio::io_service::id condition_service<Arg>::id;

} // namespace ma

#endif // MA_CONDITION_HPP