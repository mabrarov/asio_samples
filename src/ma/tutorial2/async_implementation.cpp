//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/tutorial2/do_something_handler.hpp>
#include <ma/tutorial2/async_implementation.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace tutorial2 {

namespace {

typedef boost::shared_ptr<async_implementation> async_implementation_ptr;

class forward_binder 
{
private:
  typedef forward_binder this_type;

public:
  typedef void result_type;

  typedef void (async_implementation::*func_type)(
      const do_something_handler_ptr&);

#if defined(MA_HAS_RVALUE_REFS)

  template <typename AsyncImplementationPtr, typename DoSomethingHandlerPtr>
  forward_binder(AsyncImplementationPtr&& async_implementation, 
      DoSomethingHandlerPtr&& do_something_handler, func_type func)
    : async_implementation_(std::forward<AsyncImplementationPtr>(
          async_implementation))
    , do_something_handler_(std::forward<DoSomethingHandlerPtr>(
          do_something_handler))
    , func_(func)
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  forward_binder(this_type&& other)
    : async_implementation_(std::move(other.async_implementation_))
    , do_something_handler_(std::move(other.do_something_handler_))
    , func_(other.func_)
  {
  }

  forward_binder(const this_type& other)
    : async_implementation_(other.async_implementation_)
    , do_something_handler_(other.do_something_handler_)
    , func_(other.func_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  forward_binder(const async_implementation_ptr& async_implementation, 
      const do_something_handler_ptr& do_something_handler, 
      func_type function)
    : async_implementation_(async_implementation)
    , do_something_handler_(do_something_handler)
    , func_(function)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)  

  void operator()()
  {
    ((*async_implementation_).*func_)(do_something_handler_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return context->do_something_handler_->allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, 
      this_type* context)
  {
    context->do_something_handler_->deallocate(pointer);
  }

private:
  async_implementation_ptr async_implementation_;
  do_something_handler_ptr do_something_handler_;
  func_type func_;
}; // class forward_binder

class do_something_handler_adapter
{        
private:
  typedef do_something_handler_adapter this_type;

public:
  typedef void result_type;

  explicit do_something_handler_adapter(
      const do_something_handler_ptr& do_something_handler)
    : do_something_handler_(do_something_handler)
  {
  }

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  do_something_handler_adapter(this_type&& other)
    : do_something_handler_(std::move(other.do_something_handler_))
  {
  }

  do_something_handler_adapter(const this_type& other)
    : do_something_handler_(other.do_something_handler_)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)  

  void operator()(const boost::system::error_code& error)
  {
    do_something_handler_->handle_do_something_completion(error);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return context->do_something_handler_->allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, 
      this_type* context)
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
  
public:
  typedef void result_type;

  do_something_handler_binder(const do_something_handler_ptr& do_something_handler,
    const boost::system::error_code& error)
    : do_something_handler_(do_something_handler)
    , error_(error)
  {
  }

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  do_something_handler_binder(this_type&& other)
    : do_something_handler_(std::move(other.do_something_handler_))
    , error_(std::move(other.error_))
  {
  }

  do_something_handler_binder(const this_type& other)
    : do_something_handler_(other.do_something_handler_)
    , error_(other.error_)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)  

  void operator()()
  {
    do_something_handler_->handle_do_something_completion(error_);
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return context->do_something_handler_->allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/, 
      this_type* context)
  {
    context->do_something_handler_->deallocate(pointer);
  }

private:
  do_something_handler_ptr  do_something_handler_;
  boost::system::error_code error_;
}; // class do_something_handler_binder

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

class timer_handler_binder
{
private:
  typedef timer_handler_binder this_type;
  
public:
  typedef void result_type;
  typedef void (async_implementation::*func_type)(
      const boost::system::error_code&);

  template <typename AsyncImplementationPtr>
  timer_handler_binder(func_type function, 
      AsyncImplementationPtr&& async_implementation)
    : func_(function)
    , async_implementation_(std::forward<AsyncImplementationPtr>(
          async_implementation))
  {
  } 

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  timer_handler_binder(this_type&& other)
    : func_(other.func_)
    , async_implementation_(std::move(other.async_implementation_))
  {
  }

  timer_handler_binder(const this_type& other)
    : func_(other.func_)
    , async_implementation_(other.async_implementation_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  void operator()(const boost::system::error_code& error)
  {
    ((*async_implementation_).*func_)(error);
  }

private:
  func_type            func_;
  async_implementation_ptr async_implementation_;
}; // class timer_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // anonymous namespace

async_implementation::async_implementation(boost::asio::io_service& io_service,
    const std::string& name)
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

void async_implementation::async_do_something(
    const do_something_handler_ptr& handler)
{
  strand_.post(forward_binder(shared_from_this(), handler, 
      &this_type::start_do_something));
}

void async_implementation::complete_do_something(
    const boost::system::error_code& error)
{
  do_something_handler_.post(error);
}

bool async_implementation::has_do_something_handler() const
{
  return do_something_handler_.has_target();
}

void async_implementation::start_do_something(
    const do_something_handler_ptr& handler)
{
  if (boost::optional<boost::system::error_code> result = 
      do_start_do_something())
  {
    strand_.get_io_service().post(
        do_something_handler_binder(handler, *result));
  }
  else
  {
    do_something_handler_.reset(do_something_handler_adapter(handler));
  }
}

boost::optional<boost::system::error_code> 
async_implementation::do_start_do_something()
{
  if (has_do_something_handler())
  {
    return boost::system::error_code(
        boost::asio::error::operation_not_supported);
  }

  counter_ = 10000;
  std::cout << start_message_fmt_ % name_ % counter_;

  boost::system::error_code timer_error;
  timer_.expires_from_now(boost::posix_time::seconds(3), timer_error);
  if (timer_error)
  {
    return timer_error;
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
      timer_allocator_, timer_handler_binder(&this_type::handle_timer, 
          shared_from_this()))));

#else

  timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
      timer_allocator_, boost::bind(&this_type::handle_timer, 
          shared_from_this(), boost::asio::placeholders::error))));

#endif

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

    boost::system::error_code timer_error;
    timer_.expires_from_now(boost::posix_time::seconds(1), timer_error);
    if (timer_error)
    {
      std::cout << error_end_message_fmt_ % name_ % counter_;
      complete_do_something(timer_error);
      return;
    }    

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
        timer_allocator_, timer_handler_binder(&this_type::handle_timer, 
            shared_from_this()))));

#else

    timer_.async_wait(MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
        timer_allocator_, boost::bind(&this_type::handle_timer, 
            shared_from_this(), boost::asio::placeholders::error))));

#endif

    return;
  }

  std::cout << success_end_message_fmt_ % name_ % counter_;
  complete_do_something(boost::system::error_code());
}

} // namespace tutorial2
} // namespace ma
