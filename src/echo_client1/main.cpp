//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
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
#include <ma/sp_intrusive_list.hpp>
#include <ma/shared_ptr_factory.hpp>

#if defined(MA_HAS_RVALUE_REFS)

#include <utility>

#endif

namespace {

class test_handler_base
{
private:
  typedef test_handler_base this_type;

public:
  test_handler_base()
  {
  }

  virtual int get_value() const = 0;

protected:
  ~test_handler_base()
  {
  }

  test_handler_base(const this_type&)
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }
}; // class test_handler_base

class test_handler : public test_handler_base
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

  void operator()(void)
  {
    std::cout << value_ << std::endl;
  }

  int get_value() const
  {
    return value_;
  }

private:
  int value_;
}; // class test_handler

void test_handler_storage_target(boost::asio::io_service& io_service)
{
  {
    typedef ma::handler_storage<int, test_handler_base> handler_storage_type;

    handler_storage_type handler(io_service);
    handler.store(test_handler(4));
    std::cout << handler.target()->get_value() << std::endl;
  }

  {
    typedef ma::handler_storage<int> handler_storage_type;

    handler_storage_type handler(io_service);
    handler.store(test_handler(4));
    std::cout << handler.target() << std::endl;
  }
}

class no_arg_handler
{
public:
  explicit no_arg_handler(int value)
    : value_(value)
  {
  }

  void operator()()
  {
    std::cout << value_ << std::endl;
  }

private:
  int value_;
}; // class no_arg_handler

class no_arg_handler_with_target : public test_handler_base
{
public:
  explicit no_arg_handler_with_target(int value)
    : value_(value)
  {
  }

  void operator()()
  {
    std::cout << value_ << std::endl;
  }

  int get_value() const
  {
    return value_;
  }

private:
  int value_;
}; // class no_arg_handler_with_target

class later_handler : public test_handler_base
{
public:
  class storage_holder;
  typedef boost::shared_ptr<storage_holder> storage_holder_ptr;

  explicit later_handler(int value, const storage_holder_ptr& storage_holder)
    : value_(value)
    , storage_holder_(storage_holder)
  {
  }

  void operator()(int val)
  {
    std::cout << value_ << val << std::endl;
  }

  int get_value() const
  {
    return value_;
  }

private:
  int value_;
  storage_holder_ptr storage_holder_;
}; // class later_handler

class later_handler::storage_holder : private boost::noncopyable
{
public:
  typedef ma::handler_storage<int> handler_storage_type;

  explicit storage_holder(boost::asio::io_service& io_service)
    : handler_storage_(io_service)
  {
  }

  handler_storage_type& handler_storage()
  {
    return handler_storage_;
  }

private:
  handler_storage_type handler_storage_;
}; // class later_handler::storage_holder

void test_handler_storage_arg(boost::asio::io_service& io_service)
{
  {
    typedef ma::handler_storage<void> handler_storage_type;

    handler_storage_type handler(io_service);
    handler.store(no_arg_handler(4));

    std::cout << handler.target() << std::endl;
    handler.post();
  }

  {
    typedef ma::handler_storage<void, test_handler_base> handler_storage_type;

    handler_storage_type handler(io_service);
    handler.store(no_arg_handler_with_target(4));

    std::cout << handler.target()->get_value() << std::endl;
    handler.post();
  }

  {
    boost::asio::io_service io_service;

    ma::handler_storage<void, test_handler_base> handler1(io_service);
    handler1.store(no_arg_handler_with_target(1));

    ma::handler_storage<void> handler2(io_service);
    handler2.store(no_arg_handler(2));

    ma::handler_storage<int> handler3(io_service);
    handler3.store(test_handler(3));

    ma::handler_storage<int, test_handler_base> handler4(io_service);
    handler4.store(test_handler(4));

    later_handler::storage_holder_ptr storage_holder1 =
        boost::make_shared<later_handler::storage_holder>(
            boost::ref(io_service));

    later_handler::storage_holder_ptr storage_holder2 =
        boost::make_shared<later_handler::storage_holder>(
            boost::ref(io_service));

    storage_holder1->handler_storage().store(later_handler(5, storage_holder1));
    storage_holder2->handler_storage().store(later_handler(6, storage_holder2));
  }
}

#if defined(MA_HAS_RVALUE_REFS)

void test_handler_storage_move_constructor(boost::asio::io_service& io_service)
{
  typedef ma::handler_storage<int> handler_storage_type;

  handler_storage_type handler1(io_service);
  handler1.store(test_handler(4));

  boost::thread worker_thread(boost::bind(&boost::asio::io_service::run,
      boost::ref(io_service)));

  handler_storage_type handler2(std::move(handler1));
  handler2.post(2);

  worker_thread.join();
  io_service.reset();
}

#endif // defined(MA_HAS_RVALUE_REFS)

class sp_list_test : public ma::sp_intrusive_list<sp_list_test>::base_hook
{
public:
  explicit sp_list_test(std::size_t num)
    : num_(num)
  {
  }

  ~sp_list_test()
  {
    std::cout << num_ << std::endl;
  }
private:
  std::size_t num_;
}; // sp_list_test

void test_sp_intrusive_list()
{
  ma::sp_intrusive_list<sp_list_test> sp_list;
  for (std::size_t i = 0; i != 10; ++i)
  {
    std::cout << i << std::endl;
    sp_list.push_front(boost::make_shared<sp_list_test>(i));
  }
}

class A : private boost::noncopyable
{
};

class B : private boost::noncopyable
{
public:
  explicit B(int /*data*/)
  {
  }
};

void test_shared_ptr_factory()
{
  typedef ma::shared_ptr_factory_helper<A> A_helper;
  boost::shared_ptr<A> a = boost::make_shared<A_helper>();

  typedef ma::shared_ptr_factory_helper<B> B_helper;
  boost::shared_ptr<B> b = boost::make_shared<B_helper>(42);
}

} // anonymous namespace

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

    test_handler_storage_target(io_service);
    test_handler_storage_arg(io_service);

#if defined(MA_HAS_RVALUE_REFS)
    test_handler_storage_move_constructor(io_service);
#endif

    test_sp_intrusive_list();
    test_shared_ptr_factory();

    io_service.run();

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
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}
