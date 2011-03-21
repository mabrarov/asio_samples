//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/tutorial2/do_something_handler.hpp>
#include <ma/tutorial2/async_implementation.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma
{
  namespace tutorial2
  { 
    namespace 
    {
      typedef boost::shared_ptr<async_implementation> async_implementation_ptr;

      class forward_binder 
      {
      private:
        typedef forward_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void (async_implementation::*function_type)(const do_something_handler_ptr&);

        forward_binder(const async_implementation_ptr& async_implementation,
          const do_something_handler_ptr& do_something_handler, function_type function)
          : async_implementation_(async_implementation)
          , do_something_handler_(do_something_handler)
          , function_(function)
        {
        }

#if defined(MA_HAS_RVALUE_REFS)
        forward_binder(this_type&& other)
          : async_implementation_(std::move(other.async_implementation_))
          , do_something_handler_(std::move(other.do_something_handler_))
          , function_(other.function_)          
        {
        }
#endif

        ~forward_binder()
        {
        }

        void operator()()
        {
          ((*async_implementation_).*function_)(do_something_handler_);
        }

        friend void* asio_handler_allocate(std::size_t size, this_type* context)
        {
          return context->do_something_handler_->allocate(size);
        }

        friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, this_type* context)
        {
          context->do_something_handler_->deallocate(pointer);
        }

      private:
        async_implementation_ptr async_implementation_;
        do_something_handler_ptr do_something_handler_;
        function_type function_;
      }; // class forward_binder

      class do_something_handler_adapter
      {        
      private:
        typedef do_something_handler_adapter this_type;
        this_type& operator=(const this_type&);

      public:
        explicit do_something_handler_adapter(const do_something_handler_ptr& do_something_handler)
          : do_something_handler_(do_something_handler)
        {
        }

#if defined(MA_HAS_RVALUE_REFS)
        do_something_handler_adapter(this_type&& other)
          : do_something_handler_(std::move(other.do_something_handler_))
        {
        }
#endif

        ~do_something_handler_adapter()
        {
        }

        void operator()(const boost::system::error_code& error)
        {
          do_something_handler_->handle_do_something_completion(error);
        }

        friend void* asio_handler_allocate(std::size_t size, this_type* context)
        {
          return context->do_something_handler_->allocate(size);
        }

        friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, this_type* context)
        {
          context->do_something_handler_->deallocate(pointer);
        }

      private:
        do_something_handler_ptr do_something_handler_;
      }; // class do_something_handler_adapter

      class do_something_handler_binder
      {        
      private:
        typedef do_something_handler_binder this_type;
        this_type& operator=(const this_type&);

      public:
        do_something_handler_binder(const do_something_handler_ptr& do_something_handler,
          const boost::system::error_code& error)
          : do_something_handler_(do_something_handler)
          , error_(error)
        {
        }

#if defined(MA_HAS_RVALUE_REFS)
        do_something_handler_binder(this_type&& other)
          : do_something_handler_(std::move(other.do_something_handler_))
          , error_(std::move(other.error_))
        {
        }
#endif

        ~do_something_handler_binder()
        {
        }

        void operator()()
        {
          do_something_handler_->handle_do_something_completion(error_);
        }

        friend void* asio_handler_allocate(std::size_t size, this_type* context)
        {
          return context->do_something_handler_->allocate(size);
        }

        friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, this_type* context)
        {
          context->do_something_handler_->deallocate(pointer);
        }

      private:
        do_something_handler_ptr do_something_handler_;
        boost::system::error_code error_;
      }; // class do_something_handler_binder

    } // anonymous namespace

    async_implementation::async_implementation(boost::asio::io_service& io_service, const std::string& name)
      : strand_(io_service)
      , do_something_handler_(io_service)
      , timer_(io_service)
      , name_(name)
      , start_message_fmt_("%s started. counter = %07d\n")
      , cycle_message_fmt_("%s is working. counter = %07d\n")
      , error_end_message_fmt_("%s stopped work with error. counter = %07d\n")
      , success_end_message_fmt_("%s successfully complete work. counter = %07d\n")
    {
    }

    async_implementation::~async_implementation()
    {
    }

    void async_implementation::async_do_something(const do_something_handler_ptr& handler)
    {
      strand_.post(forward_binder(shared_from_this(), handler, &this_type::call_do_something));      
    }

    void async_implementation::complete_do_something(const boost::system::error_code& error)
    {
      do_something_handler_.post(error);
    }

    bool async_implementation::has_do_something_handler() const
    {
      return do_something_handler_.has_target();
    }

    void async_implementation::call_do_something(const do_something_handler_ptr& handler)
    {
      if (boost::optional<boost::system::error_code> result = do_something())
      {
        strand_.get_io_service().post(do_something_handler_binder(handler, *result));
      }
      else
      {
        do_something_handler_.put(do_something_handler_adapter(handler));
      }
    }

    boost::optional<boost::system::error_code> async_implementation::do_something()
    {
      if (has_do_something_handler())
      {
        return boost::system::error_code(boost::asio::error::operation_not_supported);
      }      
      counter_ = 10000;
      std::cout << start_message_fmt_ % name_ % counter_;

      timer_.expires_from_now(boost::posix_time::seconds(3));
      timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(timer_allocator_,
        boost::bind(&this_type::handle_timer, shared_from_this(), boost::asio::placeholders::error))));

      return boost::optional<boost::system::error_code>();    
    }

    void async_implementation::handle_timer(const boost::system::error_code& error)
    {
      if (!has_do_something_handler())
      {
        return;
      }      
      if (error)
      {
        std::cout << error_end_message_fmt_ % name_ % counter_;
        complete_do_something(error);
        return;
      }
      if (counter_)
      {        
        --counter_;
        std::cout << cycle_message_fmt_ % name_ % counter_;

        timer_.expires_from_now(boost::posix_time::milliseconds(1));
        timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(timer_allocator_,
          boost::bind(&this_type::handle_timer, shared_from_this(), boost::asio::placeholders::error))));
        return;
      }
      std::cout << success_end_message_fmt_ % name_ % counter_;
      complete_do_something(boost::system::error_code());
    }

  } // namespace tutorial2
} // namespace ma
