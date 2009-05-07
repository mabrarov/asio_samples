//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_HPP
#define MA_HANDLER_STORAGE_HPP

#include <utility>
#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/handler_invoke_helpers.hpp>

namespace ma
{
  template <typename Arg1>
  class handler_storage : private boost::noncopyable
  {
  private:    
    typedef handler_storage<Arg1> this_type;

    class handler_base
    {
    public:
      typedef void (*invoke_func_type)(handler_base*, const Arg1&);
      typedef void (*destroy_func_type)(handler_base*);      

      explicit handler_base(invoke_func_type invoke_func, destroy_func_type destroy_func)
        : invoke_func_(invoke_func)
        , destroy_func_(destroy_func)
      {
      }

      void invoke(const Arg1& arg1)
      {
        invoke_func_(this, arg1);
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
      explicit handler_wrapper(Handler handler)
        : handler_base(&this_type::do_invoke, &this_type::do_destroy)
        , handler_(handler)
      {
      }

      static void do_invoke(handler_base* base, const Arg1& arg1)
      {
        // Take ownership of the handler object.          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);          

        // Make a copy of the handler so that the memory can be deallocated before
        // the upcall is made.
        Handler handler(h->handler_);          

        // Free the memory associated with the handler.
        ptr.reset();          

        // Make the upcall.
        ma_invoke_helpers::invoke(
          boost::asio::detail::bind_handler(handler, arg1), &handler);
        //handler(error);
      }

      static void do_destroy(handler_base* base)
      {          
        this_type* h(static_cast<this_type*>(base));
        typedef boost::asio::detail::handler_alloc_traits<Handler, this_type> alloc_traits;
        boost::asio::detail::handler_ptr<alloc_traits> ptr(h->handler_, h);

        // A sub-object of the handler may be the true owner of the memory
        // associated with the handler. Consequently, a local copy of the handler
        // is required to ensure that any owning sub-object remains valid until
        // after we have deallocated the memory here.
        Handler handler(h->handler_);
        (void)handler;

        // Free the memory associated with the handler.
        ptr.reset();
      }

    private:
      Handler handler_;
    }; // class handler_wrapper                   

  public:
    typedef void result_type;

    explicit handler_storage()
      : handler_ptr_(0)
    {
    }

    template <typename Handler>
    explicit handler_storage(Handler handler)
      : handler_ptr_(0)
    {
      typedef handler_wrapper<Handler> value_type;
      typedef boost::asio::detail::handler_alloc_traits<Handler, value_type> alloc_traits;        
      // Allocate raw memory for handler
      boost::asio::detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
      // Wrap local handler and copy wrapper into allocated memory
      boost::asio::detail::handler_ptr<alloc_traits> ptr(raw_ptr, handler);

      // Take the ownership
      handler_ptr_ = ptr.release();
    }

    ~handler_storage()
    {
      if (handler_ptr_)
      {              
        handler_ptr_->destroy();              
      }
    }    

    void swap(this_type& rhs)
    {
      std::swap(handler_ptr_, rhs.handler_ptr_);      
    }

    operator bool() const
    {
      return 0 != handler_ptr_;
    }

    void operator()(const Arg1& arg1)
    { 
      handler_base* ptr(handler_ptr_);
      handler_ptr_ = 0;
      ptr->invoke(arg1);
    }

  private:
    handler_base* handler_ptr_;
  }; // class handler_storage
          
} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP