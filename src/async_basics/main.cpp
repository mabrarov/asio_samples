//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <exception>
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
#include <ma/console_controller.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/tutorial/async_interface.hpp>
#include <ma/tutorial/async_implementation.hpp>

namespace {

typedef ma::in_place_handler_allocator<128> allocator_type;

void handle_do_something(
    const ma::tutorial::async_interface_ptr& /*active_object*/,
    const boost::system::error_code& error,
    const boost::shared_ptr<const std::string>& name,
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

void handle_program_exit(boost::asio::io_service& io_service)
{
  std::cout << "User exit request detected. Stopping work io_service...\n";
  io_service.stop();
  std::cout << "Work io_service stopped.\n";
}

} // anonymous namespace

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    std::size_t cpu_count = boost::thread::hardware_concurrency();
    std::size_t work_thread_count = cpu_count < 2 ? 2 : cpu_count;

    using boost::asio::io_service;
    io_service work_io_service(cpu_count);

    // Setup console controller
    ma::console_controller console_controller(boost::bind(handle_program_exit,
        boost::ref(work_io_service)));
    std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

    boost::thread_group work_threads;
    boost::optional<io_service::work> work_guard(
        boost::in_place(boost::ref(work_io_service)));
    for (std::size_t i = 0; i != work_thread_count; ++i)
    {
      work_threads.create_thread(boost::bind(
          &io_service::run, boost::ref(work_io_service)));
    }

    boost::format name_format("active_object%03d");
    for (std::size_t i = 0; i != 20; ++i)
    {
      boost::shared_ptr<const std::string> name =
          boost::make_shared<std::string>((name_format % i).str());

      boost::shared_ptr<allocator_type> allocator =
          boost::make_shared<allocator_type>();

      ma::tutorial::async_interface_ptr active_object =
          ma::tutorial::async_implementation::create(work_io_service, *name);

      ma::tutorial::async_interface::async_do_something(active_object,
          ma::make_custom_alloc_handler(*allocator,
              boost::bind(handle_do_something, active_object, _1, name,
                  allocator)));
    }

    work_guard = boost::none;
    work_threads.join_all();

    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}
