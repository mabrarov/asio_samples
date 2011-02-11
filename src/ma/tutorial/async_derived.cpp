//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ma/tutorial/async_derived.hpp>

namespace ma
{
  namespace tutorial
  {    
    Async_derived::Async_derived(boost::asio::io_service& io_service, const std::string& name)
      : Strand_base(boost::ref(io_service))
      , Async_base(Strand_base::member)
      , timer_(io_service)
      , name_(name)
      , start_message_fmt_("%s started. counter = %07d\n")
      , cycle_message_fmt_("%s is working. counter = %07d\n")
      , error_end_message_fmt_("%s stopped work with error. counter = %07d\n")
      , success_end_message_fmt_("%s successfully complete work. counter = %07d\n")
    {
    } // Async_derived::Async_derived

    Async_derived::~Async_derived()
    {
    } // Async_derived::~Async_derived
    
    boost::optional<boost::system::error_code> Async_derived::do_something()
    {
      if (has_do_something_handler())
      {
        return boost::asio::error::operation_not_supported;
      }      
      counter_ = 10000;
      std::cout << start_message_fmt_ % name_ % counter_;

      timer_.expires_from_now(boost::posix_time::seconds(3));
      timer_.async_wait(ma::make_custom_alloc_handler(timer_allocator_,
        boost::bind(&this_type::handle_timer, boost::static_pointer_cast<Async_derived>(shared_from_this()),
          boost::asio::placeholders::error)));

      return boost::optional<boost::system::error_code>();    
    } // Async_derived::do_something

    void Async_derived::handle_timer(const boost::system::error_code& error)
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
      } // if (error)
      if (counter_)
      {        
        --counter_;
        std::cout << cycle_message_fmt_ % name_ % counter_;

        timer_.expires_from_now(boost::posix_time::milliseconds(1));
        timer_.async_wait(ma::make_custom_alloc_handler(timer_allocator_,
          boost::bind(&this_type::handle_timer, boost::static_pointer_cast<Async_derived>(shared_from_this()),
            boost::asio::placeholders::error)));
        return;
      } // if (counter_)
      std::cout << success_end_message_fmt_ % name_ % counter_;
      complete_do_something(boost::system::error_code());
    } // Async_derived::handle_timer

  } // namespace tutorial
} // namespace ma
