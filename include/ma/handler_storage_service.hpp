//
// Copyright (c) 2008-2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_SERVICE_HPP
#define MA_HANDLER_STORAGE_SERVICE_HPP

#include <cstddef>
#include <stdexcept>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/call_traits.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/bind_asio_handler.hpp>

namespace ma
{  
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

      explicit handler_base(invoke_func_type invoke_func, destroy_func_type destroy_func)
        : invoke_func_(invoke_func)
        , destroy_func_(destroy_func)        
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
      this_type& operator=(const this_type&);    

    public:
      explicit handler_wrapper(boost::asio::io_service& io_service, Handler handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy)
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
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made.
        boost::asio::io_service& io_service(h->io_service_);
        boost::asio::io_service::work work(h->work_);
        (void) work;
        Handler handler(h->handler_);          

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        io_service.post(detail::bind_handler(handler, arg));
      }

      static void do_destroy(handler_base* base)
      {          
        this_type* h(static_cast<this_type*>(base));
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(h->handler_, h);

        // A sub-object of the handler may be the true owner of the memory
        // associated with the handler. Consequently, a local copy of the handler
        // is required to ensure that any owning sub-object remains valid until
        // after we have deallocated the memory here.
        Handler handler(h->handler_);
        (void) handler;

        // Free the memory associated with the handler.
        ptr.reset();
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
    };

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
        impl.next_ = front_;
        impl.prev_ = 0;
        if (front_)
        {
          front_->prev_ = &impl;
        }
        front_ = &impl;
      }

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
      }

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
    }    

    void construct(implementation_type& impl)
    {      
      mutex_type::scoped_lock lock(mutex_);
      impl_list_.push_front(impl);     
    }

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
    }

    template <typename Handler>
    void store(implementation_type& impl, Handler handler)
    {
      if (!shutdown_done_)
      {      
        typedef handler_wrapper<Handler> value_type;
        typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;
        // Allocate raw memory for handler
        detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
        // Wrap local handler and copy wrapper into allocated memory
        detail::handler_ptr<alloc_traits> ptr(raw_ptr, this->get_io_service(), handler);
        // Copy current handler ptr
        handler_base* handler_ptr = impl.handler_ptr_;
        // Take the ownership
        impl.handler_ptr_ = ptr.release();
        if (handler_ptr)
        {
          handler_ptr->destroy();
        }
      }
    }

    void post(implementation_type& impl, arg_param_type arg) const
    {      
      // Take the ownership
      handler_base* handler_ptr = impl.handler_ptr_;
      impl.handler_ptr_ = 0;
      if (handler_ptr)
      {
        handler_ptr->invoke(arg);
      }
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

  template <typename Arg, typename Data>
  class handler_storage_service2 : public boost::asio::io_service::service
  {
  private:
    typedef handler_storage_service2<Arg, Data> this_type;    
    typedef boost::mutex mutex_type;

  public:
    typedef Arg  argument_type;
    typedef Data data_type;

  private:    
    typedef typename boost::call_traits<argument_type>::param_type arg_param_type;
    typedef typename boost::call_traits<data_type>::param_type     data_param_type;
    typedef typename boost::call_traits<data_type>::reference      data_ref_type;

    class handler_base
    {
    private:
      typedef handler_base this_type;
      this_type& operator=(const this_type&);    

    public:
      typedef void (*invoke_func_type)(handler_base*, arg_param_type);
      typedef void (*destroy_func_type)(handler_base*);

      explicit handler_base(invoke_func_type invoke_func, 
        destroy_func_type destroy_func, data_param_type data)
        : invoke_func_(invoke_func)
        , destroy_func_(destroy_func)
        , data_(data)
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

      data_ref_type data()
      {
        return data_;
      }

    protected:
      ~handler_base()
      {
      }

    private:        
      invoke_func_type invoke_func_;
      destroy_func_type destroy_func_;
      data_type data_;
    }; // class handler_base    
      
    template <typename Handler>
    class handler_wrapper : public handler_base
    {
    private:
      typedef handler_wrapper<Handler> this_type;
      this_type& operator=(const this_type&);    

    public:
      explicit handler_wrapper(boost::asio::io_service& io_service,
        data_param_type data, Handler handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy, data)
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
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made.
        boost::asio::io_service& io_service(h->io_service_);
        boost::asio::io_service::work work(h->work_);
        (void) work;
        Handler handler(h->handler_);          

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        io_service.post(detail::bind_handler(handler, arg));
      }

      static void do_destroy(handler_base* base)
      {          
        this_type* h(static_cast<this_type*>(base));
        typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        detail::handler_ptr<alloc_traits> ptr(h->handler_, h);

        // A sub-object of the handler may be the true owner of the memory
        // associated with the handler. Consequently, a local copy of the handler
        // is required to ensure that any owning sub-object remains valid until
        // after we have deallocated the memory here.
        Handler handler(h->handler_);
        (void) handler;

        // Free the memory associated with the handler.
        ptr.reset();
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
      friend class handler_storage_service2<argument_type, data_type>;
      implementation_type* prev_;
      implementation_type* next_;      
      handler_base* handler_ptr_;
    };

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
        impl.next_ = front_;
        impl.prev_ = 0;
        if (front_)
        {
          front_->prev_ = &impl;
        }
        front_ = &impl;
      }

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
      }

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
    explicit handler_storage_service2(boost::asio::io_service& io_service)
      : boost::asio::io_service::service(io_service)      
      , shutdown_done_(false)
    {
    }

    ~handler_storage_service2()
    {
    }

    void shutdown_service()
    {   
      shutdown_done_ = true;
      while (!impl_list_.empty())
      {
        destroy(*impl_list_.front());
      }      
    }    

    void construct(implementation_type& impl)
    {      
      mutex_type::scoped_lock lock(mutex_);
      impl_list_.push_front(impl);     
    }

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
    }

    template <typename Handler>
    void store(implementation_type& impl, data_param_type data, Handler handler)
    {
      if (!shutdown_done_)
      {      
        typedef handler_wrapper<Handler> value_type;
        typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;
        // Allocate raw memory for handler
        detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
        // Wrap local handler and copy wrapper into allocated memory
        detail::handler_ptr<alloc_traits> ptr(raw_ptr, 
          this->get_io_service(), data, handler);
        // Copy current handler ptr
        handler_base* handler_ptr = impl.handler_ptr_;
        // Take the ownership
        impl.handler_ptr_ = ptr.release();
        if (handler_ptr)
        {
          handler_ptr->destroy();
        }
      }
    }

    void post(implementation_type& impl, arg_param_type arg) const
    {      
      // Take the ownership
      handler_base* handler_ptr = impl.handler_ptr_;
      impl.handler_ptr_ = 0;
      if (handler_ptr)
      {
        handler_ptr->invoke(arg);
      }
    }

    data_ref_type data(implementation_type& impl) const
    {
      if (impl.handler_ptr_)
      {
        return impl.handler_ptr_->data();
      }
      else
      {
        boost::throw_exception(std::runtime_error("call to data() of an empty handler storage"));
      }
    }

    bool has_target(const implementation_type& impl) const
    {
      return 0 != impl.handler_ptr_;
    }

  private:
    mutex_type mutex_;
    impl_list impl_list_;
    bool shutdown_done_;
  }; // class handler_storage_service2

  template <typename Arg, typename Data>
  boost::asio::io_service::id handler_storage_service2<Arg, Data>::id;


} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP