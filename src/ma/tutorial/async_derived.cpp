//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/tutorial/async_derived.hpp>

namespace ma {
namespace tutorial {

namespace {

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

class timer_handler_binder
{
private:
  typedef timer_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (async_derived::*func_type)(const boost::system::error_code&);

  template <typename AsyncDerivedPtr>
  timer_handler_binder(func_type func, AsyncDerivedPtr&& async_derived)
    : func_(func)
    , async_derived_(std::forward<AsyncDerivedPtr>(async_derived))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  timer_handler_binder(this_type&& other)
    : func_(other.func_)
    , async_derived_(std::move(other.async_derived_))
  {
  }

  timer_handler_binder(const this_type& other)
    : func_(other.func_)
    , async_derived_(other.async_derived_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  void operator()(const boost::system::error_code& error)
  {
    ((*async_derived_).*func_)(error);
  }

private:
  func_type func_;
  async_derived_ptr async_derived_;
}; // class timer_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // anonymous namespace

async_derived_ptr async_derived::create(boost::asio::io_service& io_service, 
    const std::string& name)
{
  typedef shared_ptr_factory_helper<this_type> helper;
  return boost::make_shared<helper>(boost::ref(io_service), name);
}

async_derived::async_derived(boost::asio::io_service& io_service,
    const std::string& name)
  : strand_base(boost::ref(io_service))
  , async_base(strand_base::member)
  , timer_(io_service)
  , name_(name)
  , start_message_fmt_("%s started. counter = %07d\n")
  , cycle_message_fmt_("%s is working. counter = %07d\n")
  , error_end_message_fmt_("%s stopped work with error. counter = %07d\n")
  , success_end_message_fmt_("%s successfully complete work. counter = %07d\n")
{
}

async_derived::~async_derived()
{
}

async_derived::async_base_ptr async_derived::get_shared_base()
{
  return shared_from_this();
}

boost::optional<boost::system::error_code>
async_derived::do_start_do_something()
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

  timer_.async_wait(MA_STRAND_WRAP(strand_base::member,
      ma::make_custom_alloc_handler(timer_allocator_, timer_handler_binder(
          &this_type::handle_timer, shared_from_this()))));

#else

  timer_.async_wait(MA_STRAND_WRAP(strand_base::member,
      ma::make_custom_alloc_handler(timer_allocator_, boost::bind(
          &this_type::handle_timer, shared_from_this(), _1))));

#endif

  return boost::optional<boost::system::error_code>();
}

void async_derived::handle_timer(const boost::system::error_code& error)
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
    timer_.expires_from_now(boost::posix_time::milliseconds(1), timer_error);
    if (timer_error)
    {
      std::cout << error_end_message_fmt_ % name_ % counter_;
      complete_do_something(timer_error);
      return;
    }

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    timer_.async_wait(MA_STRAND_WRAP(strand_base::member,
        ma::make_custom_alloc_handler(timer_allocator_, timer_handler_binder(
            &this_type::handle_timer, shared_from_this()))));

#else

    timer_.async_wait(MA_STRAND_WRAP(strand_base::member,
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
