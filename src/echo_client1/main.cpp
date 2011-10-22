//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <iostream>
#include <exception>
#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/echo/client1/session.hpp>
#include <ma/console_controller.hpp>
#include <ma/handler_storage.hpp>

#if defined(MA_HAS_RVALUE_REFS)

#include <utility>

namespace {

class test_handler
{
public:
  explicit test_handler(int value)
    : value_(value)
  {
  }

  void operator()(int val) 
  {
    std::cout << value_ << val << std::endl;
  }

private:
  int value_;
}; // class test_handler

void test_handler_storage_move_constructor(boost::asio::io_service& io_service)
{
  typedef ma::handler_storage<int> handler_storage_type;

  handler_storage_type handler1(io_service);  
  handler1.reset(test_handler(4));

  boost::thread worker_thread(boost::bind(&boost::asio::io_service::run,
      boost::ref(io_service)));

  handler_storage_type handler2(std::move(handler1));
  handler2.post(2);

  worker_thread.join();
  io_service.reset();
}

} // anonymous namespace

#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{   
  try 
  {
    boost::program_options::options_description options_description("Allowed options");
    options_description.add_options()
      (
        "help", 
        "produce help message"
      );

    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), 
      options_values);
    boost::program_options::notify(options_values);

    if (options_values.count("help"))
    {
      std::cout << options_description;
      return EXIT_SUCCESS;
    }

    std::size_t cpu_count = boost::thread::hardware_concurrency();
    std::size_t session_thread_count = cpu_count > 1 ? cpu_count : 2;
    boost::asio::io_service io_service(session_thread_count);

#if defined(MA_HAS_RVALUE_REFS)    
    test_handler_storage_move_constructor(io_service);
#endif    
    
    //todo
    return EXIT_SUCCESS;
  }
  catch (const boost::program_options::error& e)
  {    
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {    
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
} // _tmain
