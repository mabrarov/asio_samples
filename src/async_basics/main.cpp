//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/tutorial/async_derived.hpp>

void handle_do_something(const boost::system::error_code& error)
{
  //bla..bla..bla
  if (error)
  {
    std::cout << "Error" << std::endl;
  }
  else
  {
    std::cout << "Done" << std::endl;
  }
}

int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
{     
  ma::in_place_handler_allocator<128> handler_allocator;

  using boost::asio::io_service;
  io_service work_io_service;
  boost::scoped_ptr<io_service::work> work_io_service_guard(new io_service::work(work_io_service));
  boost::thread work_thread(boost::bind(&io_service::run, boost::ref(work_io_service)));

  using ma::tutorial::Async_base;
  using ma::tutorial::Async_derived;
  boost::shared_ptr<Async_base> active_object(boost::make_shared<Async_derived>(boost::ref(work_io_service))); 
  active_object->async_do_something(ma::make_custom_alloc_handler(handler_allocator,
    boost::bind(&handle_do_something, _1)));
  
  work_io_service_guard.reset();
  work_thread.join();
  
  return EXIT_SUCCESS;
}