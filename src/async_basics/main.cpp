//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/tutorial/async_derived.hpp>

typedef ma::in_place_handler_allocator<128> allocator_type;

void handle_do_something(const boost::system::error_code& error,
  const boost::shared_ptr<std::string>& name, 
  const boost::shared_ptr<allocator_type>& /*allocator*/)
{  
  if (error)
  {
    std::cout << boost::format("%s complete work with error\n") % *name;
  }
  else
  {
    std::cout << boost::format("%s successfully complete work\n") % *name;
  }
}

int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
{     
  try
  {  
    std::size_t cpu_count = boost::thread::hardware_concurrency();
    std::size_t work_thread_count = cpu_count < 2 ? 2 : cpu_count;

    using boost::asio::io_service;
    io_service work_io_service(cpu_count);

    boost::optional<io_service::work> work_io_service_guard(boost::in_place(boost::ref(work_io_service)));
    boost::thread_group work_threads;
    for (std::size_t i = 0; i != work_thread_count; ++i)
    {
      work_threads.create_thread(boost::bind(&io_service::run, boost::ref(work_io_service)));
    }

    boost::format name_format("active_object%03d");
    for (std::size_t i = 0; i != 20; ++i)
    { 
      using boost::shared_ptr;
      shared_ptr<std::string> name = boost::make_shared<std::string>((name_format % i).str());
      shared_ptr<allocator_type> allocator = boost::make_shared<allocator_type>();

      using ma::tutorial::async_base;
      using ma::tutorial::async_derived;
      shared_ptr<async_base> active_object = boost::make_shared<async_derived>(boost::ref(work_io_service), *name); 
      active_object->async_do_something(ma::make_custom_alloc_handler(*allocator,
        boost::bind(&handle_do_something, _1, name, allocator)));
    }

    work_io_service_guard.reset();
    work_threads.join_all();

    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;    
  }
  return EXIT_FAILURE;
}