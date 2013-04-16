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
#include <cstddef>
#include <iostream>
#include <exception>
#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/in_place_factory.hpp> 
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_storage.hpp>
#include <ma/lockable_wrapped_handler.hpp>

#if defined(MA_HAS_RVALUE_REFS)

#include <utility>

#endif

namespace ma {
namespace test {

namespace lockable_wrapper {

void run_test();

} // namespace lockable_wrapper

namespace handler_storage_service_destruction {

void run_test();

} // namespace handler_storage_service_destruction

namespace handler_storage_target {

void run_test();

} // namespace handler_storage_target

} // namespace test
} // namespace ma

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    ma::test::lockable_wrapper::run_test();
    ma::test::handler_storage_service_destruction::run_test();
    ma::test::handler_storage_target::run_test();

//    test_handler_storage_arg(io_service);
//
//#if defined(MA_HAS_RVALUE_REFS)
//    test_handler_storage_move_constructor(io_service);
//#endif
//
//    test_sp_intrusive_list();
//    test_shared_ptr_factory();

    return EXIT_SUCCESS;
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

namespace ma {
namespace test {

class io_service_pool : private boost::noncopyable
{
public:
  io_service_pool(boost::asio::io_service& io_service, std::size_t size)
    : work_guard_(boost::in_place(boost::ref(io_service)))
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      threads_.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service));
    }
  }

  ~io_service_pool()
  {
    boost::asio::io_service& io_service = work_guard_->get_io_service();
    work_guard_ = boost::none;
    io_service.stop();
    threads_.join_all();
  }

private:
  typedef boost::optional<boost::asio::io_service::work> optional_io_work;

  optional_io_work    work_guard_;
  boost::thread_group threads_;
}; // class io_service_pool

class wall : boost::noncopyable 
{
private:
  typedef wall this_type;
  typedef boost::mutex mutex_type;
  typedef boost::unique_lock<mutex_type> lock_type;
  typedef boost::condition_variable condition_variable_type;
  
public:
  typedef std::size_t value_type;

  explicit wall(value_type value = 0)
    : value_(value)
  {
  }

  void wait()
  {
    lock_type lock(mutex_);
    while (value_)
    {
      condition_variable_.wait(lock);
    }
  }

  void inc()
  {
    lock_type lock(mutex_);
    BOOST_ASSERT_MSG(value_ != (std::numeric_limits<value_type>::max)(),
        "Value is too large. Overflow");
    ++value_;
  }

  void dec()
  {
    lock_type lock(mutex_);
    BOOST_ASSERT_MSG(value_, "Value is too small. Underflow");
    --value_;
    if (!value_)
    {
      condition_variable_.notify_one();
    }
  }

private:
  mutex_type mutex_;
  condition_variable_type condition_variable_;
  value_type value_;
}; // class wall

namespace lockable_wrapper {

void run_test()
{
  std::cout << "*** ma::test::lockable_wrapper ***" << std::endl;

  std::size_t cpu_count = boost::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);
  
  boost::mutex mutex;
  std::string data;
  
  {
    boost::lock_guard<boost::mutex> lock_guard(mutex);
    data = "Test";
  }  

  wall done_wall(1);  
  io_service.post(ma::make_lockable_wrapped_handler(mutex, [&data, &done_wall]()
  {
    data = data + data;
    done_wall.dec();
  }));

  {
    boost::lock_guard<boost::mutex> data_guard(mutex);
    data = "Zero";
  }

  done_wall.wait();

  {
    boost::lock_guard<boost::mutex> lock_guard(mutex);
    std::cout << data << std::endl;
  }  
  
} // run_test

} // namespace lockable_wrapper

namespace handler_storage_service_destruction {

typedef ma::handler_storage<void> handler_storage_type;
typedef boost::shared_ptr<handler_storage_type> handler_storage_ptr;

class handler
{
private:
  typedef handler this_type;

public:
  handler(int num, const handler_storage_ptr& handler_storage)
    : num_(num)
    , handler_storage_(handler_storage)
  {
  }

  ~handler()
  {
    std::cout
        << "ma::test::handler_storage_service_destruction::handler::~handler()"
        << std::endl << "num_: " << num_ << std::endl;
  }

  handler(const this_type& other)
    : num_(other.num_)
    , handler_storage_(other.handler_storage_)
  {
  }

#if defined(MA_HAS_RVALUE_REFS)
  handler(this_type&& other)
    : num_(other.num_)
    , handler_storage_(std::move(other.handler_storage_))
  {
    other.num_ = 0;
  }
#endif

  void operator()()
  {
    std::cout
        << "ma::test::handler_storage_service_destruction::handler::operator()"
        << std::endl << "num_: " << num_ << std::endl;
  }  
  
private:
  int num_;
  handler_storage_ptr handler_storage_;
}; // class handler

void run_test()
{
  std::cout << "*** ma::test::handler_storage_service_destruction ***" 
      << std::endl;
  std::size_t cpu_count = boost::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
 
  handler_storage_ptr handler_storage1(
      boost::make_shared<handler_storage_type>(boost::ref(io_service)));
  handler_storage1->store(handler(1, handler_storage1));

  handler_storage_ptr handler_storage2(
      boost::make_shared<handler_storage_type>(boost::ref(io_service)));
  handler_storage2->store(handler(2, handler_storage1));

  {
    handler_storage_ptr handler_storage(
        boost::make_shared<handler_storage_type>(boost::ref(io_service)));
    handler_storage->store(handler(3, handler_storage));
  }

  handler_storage_ptr handler_storage3(
      boost::make_shared<handler_storage_type>(boost::ref(io_service)));
  handler_storage3->store(handler(4, handler_storage3));
}

} // namespace handler_storage_service_destruction

namespace handler_storage_target {

class handler_base
{
private:
  typedef handler_base this_type;

public:
  handler_base()
  {
  }

  virtual int get_value() const = 0;

protected:
  ~handler_base()
  {
  }

  handler_base(const this_type&)
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }
}; // class handler_base

class handler : public handler_base
{
public:
  explicit handler(int value)
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
}; // class handler

void run_test()
{
  std::cout << "*** ma::test::handler_storage_target ***" << std::endl;

  std::size_t cpu_count = boost::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);  

  {
    typedef ma::handler_storage<int, handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(handler(4));
    std::cout << handler_storage.target()->get_value() << std::endl;
  }

  {
    typedef ma::handler_storage<int> handler_storage1_type;

    handler_storage1_type handler_storage1(io_service);
    handler_storage1.store(handler(4));
    std::cout << handler_storage1.target() << std::endl;
  
#if defined(MA_HAS_LAMBDA)
    {
      typedef ma::handler_storage<void> handler_storage2_type;

      wall done_wall;

      handler_storage2_type handler_storage2(io_service);
      handler_storage2.store([&done_wall]
      {
          std::cout << "in lambda" << std::endl;
          done_wall.dec();
      });
      std::cout << handler_storage2.target() << std::endl;

      done_wall.inc();
      handler_storage2.post();
      done_wall.wait();
    }
#endif
  }  
} // run_test

} // namespace handler_storage_target

} // namespace test
} // namespace ma

//namespace {
//
//class no_arg_handler
//{
//public:
//  explicit no_arg_handler(int value)
//    : value_(value)
//  {
//  }
//
//  void operator()()
//  {
//    std::cout << value_ << std::endl;
//  }
//
//private:
//  int value_;
//}; // class no_arg_handler
//
//class no_arg_handler_with_target : public test_handler_base
//{
//public:
//  explicit no_arg_handler_with_target(int value)
//    : value_(value)
//  {
//  }
//
//  void operator()()
//  {
//    std::cout << value_ << std::endl;
//  }
//
//  int get_value() const
//  {
//    return value_;
//  }
//
//private:
//  int value_;
//}; // class no_arg_handler_with_target
//
//class later_handler : public test_handler_base
//{
//public:
//  class storage_holder;
//  typedef boost::shared_ptr<storage_holder> storage_holder_ptr;
//
//  explicit later_handler(int value, const storage_holder_ptr& storage_holder)
//    : value_(value)
//    , storage_holder_(storage_holder)
//  {
//  }
//
//  void operator()(int val)
//  {
//    std::cout << value_ << val << std::endl;
//  }
//
//  int get_value() const
//  {
//    return value_;
//  }
//
//private:
//  int value_;
//  storage_holder_ptr storage_holder_;
//}; // class later_handler
//
//class later_handler::storage_holder : private boost::noncopyable
//{
//public:
//  typedef ma::handler_storage<int> handler_storage_type;
//
//  explicit storage_holder(boost::asio::io_service& io_service)
//    : handler_storage_(io_service)
//  {
//  }
//
//  handler_storage_type& handler_storage()
//  {
//    return handler_storage_;
//  }
//
//private:
//  handler_storage_type handler_storage_;
//}; // class later_handler::storage_holder
//
//void test_handler_storage_arg(boost::asio::io_service& io_service)
//{
//  std::cout << "*** test_handler_storage_arg ***" << std::endl;
//  {
//    typedef ma::handler_storage<void> handler_storage_type;
//
//    handler_storage_type handler(io_service);
//    handler.store(no_arg_handler(4));
//
//    std::cout << handler.target() << std::endl;
//    handler.post();
//  }
//
//  {
//    typedef ma::handler_storage<void, test_handler_base> handler_storage_type;
//
//    handler_storage_type handler(io_service);
//    handler.store(no_arg_handler_with_target(4));
//
//    std::cout << handler.target()->get_value() << std::endl;
//    handler.post();
//  }
//
//  {
//    boost::asio::io_service io_service;
//
//    ma::handler_storage<void, test_handler_base> handler1(io_service);
//    handler1.store(no_arg_handler_with_target(1));
//
//    ma::handler_storage<void> handler2(io_service);
//    handler2.store(no_arg_handler(2));
//
//    ma::handler_storage<int> handler3(io_service);
//    handler3.store(test_handler(3));
//
//    ma::handler_storage<int, test_handler_base> handler4(io_service);
//    handler4.store(test_handler(4));
//
//    later_handler::storage_holder_ptr storage_holder1 =
//        boost::make_shared<later_handler::storage_holder>(
//            boost::ref(io_service));
//
//    later_handler::storage_holder_ptr storage_holder2 =
//        boost::make_shared<later_handler::storage_holder>(
//            boost::ref(io_service));
//
//    storage_holder1->handler_storage().store(later_handler(5, storage_holder1));
//    storage_holder2->handler_storage().store(later_handler(6, storage_holder2));
//  }
//}
//
//#if defined(MA_HAS_RVALUE_REFS)
//
//void test_handler_storage_move_constructor(boost::asio::io_service& io_service)
//{
//  std::cout << "*** test_handler_storage_move_constructor ***" << std::endl;
//
//  typedef ma::handler_storage<int> handler_storage_type;
//
//  handler_storage_type handler1(io_service);
//  handler1.store(test_handler(4));
//  
//  handler_storage_type handler2(std::move(handler1));
//  handler2.post(2);
//}
//
//#endif // defined(MA_HAS_RVALUE_REFS)
//
//class sp_list_test : public ma::sp_intrusive_list<sp_list_test>::base_hook
//{
//public:
//  explicit sp_list_test(std::size_t num)
//    : num_(num)
//  {
//  }
//
//  ~sp_list_test()
//  {
//    std::cout << num_ << std::endl;
//  }
//private:
//  std::size_t num_;
//}; // sp_list_test
//
//void test_sp_intrusive_list()
//{
//  std::cout << "*** test_sp_intrusive_list ***" << std::endl;
//  ma::sp_intrusive_list<sp_list_test> sp_list;
//  for (std::size_t i = 0; i != 10; ++i)
//  {
//    std::cout << i << std::endl;
//    sp_list.push_front(boost::make_shared<sp_list_test>(i));
//  }
//}
//
//class A : private boost::noncopyable
//{
//public:
//  static boost::shared_ptr<A> create()
//  {
//    typedef ma::shared_ptr_factory_helper<A> shared_ptr_helper;
//    return boost::make_shared<shared_ptr_helper>();
//  }
//
//protected:
//  ~A()
//  {
//  }
//};
//
//class B : private boost::noncopyable
//{
//public:
//  static boost::shared_ptr<B> create(int data)
//  {
//    typedef ma::shared_ptr_factory_helper<B> shared_ptr_helper;
//    return boost::make_shared<shared_ptr_helper>(data);
//  }
//
//protected:
//  explicit B(int /*data*/)
//  {
//  }
//
//  ~B()
//  {
//  }
//};
//
//void test_shared_ptr_factory()
//{  
//  std::cout << "*** test_shared_ptr_factory ***" << std::endl;
//  boost::shared_ptr<const A> a = A::create();  
//  boost::shared_ptr<B> b = B::create(42);
//}
//
//class io_service_pool : private boost::noncopyable
//{
//public:
//  io_service_pool(boost::asio::io_service& io_service, std::size_t size)
//    : work_guard_(boost::in_place(boost::ref(io_service)))
//  {
//    for (std::size_t i = 0; i < size; ++i)
//    {
//      threads_.create_thread(
//        boost::bind(&boost::asio::io_service::run, &io_service));
//    }
//  }
//
//  ~io_service_pool()
//  {
//    work_guard_ = boost::none;
//    threads_.join_all();
//  }
//
//private:
//  typedef boost::optional<boost::asio::io_service::work> optional_io_work;
//
//  optional_io_work    work_guard_;
//  boost::thread_group threads_;
//}; // class io_service_pool
//
//} // anonymous namespace
