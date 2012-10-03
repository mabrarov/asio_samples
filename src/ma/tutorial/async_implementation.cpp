//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/tutorial/async_implementation.hpp>

namespace ma {
namespace tutorial {

namespace {

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

class timer_handler_binder
{
private:
  typedef timer_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (async_implementation::*func_type)(
      const boost::system::error_code&);

  template <typename AsyncImplementationPtr>
  timer_handler_binder(func_type func,
      AsyncImplementationPtr&& async_implementation)
    : func_(func)
    , async_implementation_(
          std::forward<AsyncImplementationPtr>(async_implementation))
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
  func_type func_;
  async_implementation_ptr async_implementation_;
}; // class timer_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // anonymous namespace

async_implementation_ptr async_implementation::create(
    boost::asio::io_service& io_service, const std::string& name)
{
  typedef shared_ptr_factory_helper<this_type> helper;
  return boost::make_shared<helper>(boost::ref(io_service), name);
}

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

boost::asio::io_service::strand& async_implementation::strand()
{
  return strand_;
}

async_interface::do_something_handler_storage_type&
async_implementation::do_something_handler_storage()
{
  return do_something_handler_;
}

boost::optional<boost::system::error_code>
async_implementation::start_do_something()
{
  if (has_do_something_handler())
  {
    return boost::system::error_code(
        boost::asio::error::operation_not_supported);
  }

  counter_ = 10000;
  std::cout << start_message_fmt_ % name_ % counter_;

  boost::system::error_code timer_error;
  timer_.expires_from_now(
      to_steady_deadline_timer_duration(boost::posix_time::seconds(3)),
      timer_error);
  if (timer_error)
  {
    return timer_error;
  }

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  timer_.async_wait(MA_STRAND_WRAP(strand_,
      ma::make_custom_alloc_handler(timer_allocator_, timer_handler_binder(
          &this_type::handle_timer, shared_from_this()))));

#else

  timer_.async_wait(MA_STRAND_WRAP(strand_,
      ma::make_custom_alloc_handler(timer_allocator_, boost::bind(
          &this_type::handle_timer, shared_from_this(), _1))));

#endif

  return boost::optional<boost::system::error_code>();
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
    timer_.expires_from_now(
        to_steady_deadline_timer_duration(boost::posix_time::milliseconds(1)),
        timer_error);
    if (timer_error)
    {
      std::cout << error_end_message_fmt_ % name_ % counter_;
      complete_do_something(timer_error);
      return;
    }

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_, timer_handler_binder(
            &this_type::handle_timer, shared_from_this()))));

#else

    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_, boost::bind(
            &this_type::handle_timer, shared_from_this(), _1))));

#endif

    return;
  }

  // Continue work
  std::cout << success_end_message_fmt_ % name_ % counter_;
  complete_do_something(boost::system::error_code());
}

} // namespace tutorial
} // namespace ma
