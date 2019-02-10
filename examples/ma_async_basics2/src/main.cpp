//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <iostream>
#include <exception>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/console_close_guard.hpp>
#include <ma/thread_group.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include "async_interface.hpp"
#include "async_implementation.hpp"
#include "do_something_handler.hpp"

namespace {

class do_something_handler_implementation
  : public ma::tutorial2::do_something_handler
  , private boost::noncopyable
{
public:
  do_something_handler_implementation(
      const ma::tutorial2::async_interface_ptr& async_interface,
      const std::string& name)
    : async_interface_(async_interface)
    , name_(name)
    , allocator_()
  {
  }

  ~do_something_handler_implementation()
  {
  }

  virtual void handle_do_something_completion(
      const boost::system::error_code& error)
  {
    if (error)
    {
      std::cout << boost::format("%s complete work with error\n") % name_;
    }
    else
    {
      std::cout << boost::format("%s successfully complete work\n") % name_;
    }
  }

  virtual void* allocate(std::size_t size)
  {
    return allocator_.allocate(size);
  }

  virtual void deallocate(void* pointer)
  {
    allocator_.deallocate(pointer);
  }

private:
  // For example only: handler "holds up" active object itself
  ma::tutorial2::async_interface_ptr async_interface_;
  std::string name_;
  ma::in_place_handler_allocator<128> allocator_;
}; // class do_something_handler_implementation

void handle_program_exit(boost::asio::io_service& io_service)
{
  std::cout << "User exit request detected. Stopping work io_service...\n";
  io_service.stop();
  std::cout << "Work io_service stopped.\n";
}

} // anonymous namespace

#if defined(MA_WIN32_TMAIN)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    std::size_t cpu_count = ma::detail::thread::hardware_concurrency();
    std::size_t work_thread_count = cpu_count < 2 ? 2 : cpu_count;

    using boost::asio::io_service;
    io_service work_io_service(ma::to_io_context_concurrency_hint(cpu_count));

    // Setup console controller
    ma::console_close_guard console_close_guard(ma::detail::bind(
        handle_program_exit, ma::detail::ref(work_io_service)));

    std::cout << "Press Ctrl+C to exit.\n";

    ma::thread_group work_threads;
    boost::optional<io_service::work> work_guard(
        boost::in_place(ma::detail::ref(work_io_service)));
    for (std::size_t i = 0; i != work_thread_count - 1; ++i)
    {
      work_threads.create_thread(ma::detail::bind(
          static_cast<std::size_t (boost::asio::io_service::*)(void)>(
              &io_service::run),
          ma::detail::ref(work_io_service)));
    }

    boost::format name_format("active_object%03d");
    for (std::size_t i = 0; i != 20; ++i)
    {
      std::string name = (name_format % i).str();

      ma::tutorial2::async_interface_ptr active_object =
          ma::tutorial2::async_implementation::create(work_io_service, name);

      active_object->async_do_something(
          ma::detail::make_shared<do_something_handler_implementation>(
              active_object, name));
    }

    work_guard = boost::none;
    work_io_service.run();
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
