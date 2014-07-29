//
// Copyright (c) 2010-2014 Marat Abrarov (abrarov@gmail.com)
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
#include <limits>
#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <ma/config.hpp>
#include <ma/memory.hpp>
#include <ma/functional.hpp>
#include <ma/thread.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_storage.hpp>
#include <ma/lockable_wrapped_handler.hpp>
#include <ma/thread_group.hpp>
#include <ma/detail/latch.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

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

namespace handler_storage_arg {

void run_test();

} // namespace handler_storage_arg

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
    ma::test::handler_storage_arg::run_test();

    //test_handler_storage_arg(io_service);

//#if defined(MA_HAS_RVALUE_REFS)
//    test_handler_storage_move_constructor(io_service);
//#endif

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

void count_down(ma::detail::latch& latch)
{
  latch.count_down();
}

class io_service_pool : private boost::noncopyable
{
public:
  io_service_pool(boost::asio::io_service& io_service, std::size_t size)
    : work_guard_(boost::in_place(MA_REF(io_service)))
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      threads_.create_thread(MA_BIND(
          static_cast<std::size_t (boost::asio::io_service::*)(void)>(
              &boost::asio::io_service::run),
          &io_service));
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
  ma::thread_group threads_;
}; // class io_service_pool

namespace lockable_wrapper {

typedef std::string data_type;
typedef MA_FUNCTION<void (void)> continuation;

void mutating_func1(data_type& d, const continuation& cont)
{
  d += " 1 ";
  cont();
}

void mutating_func2(data_type& d, const continuation& cont)
{
  d += " 2 ";
  cont();
}

void mutating_func3(data_type& d, const continuation& cont)
{
  d += " 3 ";
  cont();
}

void run_test()
{
  typedef MA_MUTEX                  mutex_type;
  typedef MA_LOCK_GUARD<mutex_type> lock_guard_type;

  std::cout << "*** ma::test::lockable_wrapper ***" << std::endl;

  std::size_t cpu_count = MA_THREAD::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);
   

  mutex_type mutex;
  std::string data;
  {
    lock_guard_type lock_guard(mutex);
    data = "0";
  }

  detail::latch done_latch(1);
  {
    lock_guard_type data_guard(mutex);

    io_service.post(ma::make_lockable_wrapped_handler(mutex, MA_BIND(
        mutating_func1, MA_REF(data), continuation(
            MA_BIND(count_down, MA_REF(done_latch))))));

    detail::this_thread::sleep(boost::posix_time::seconds(5));
    data = "Zero";
  }
  done_latch.wait();

  {
    lock_guard_type lock_guard(mutex);
    BOOST_ASSERT_MSG("Zero 1 " == data, "Invalid value of data");
  }
} // run_test

} // namespace lockable_wrapper

namespace handler_storage_service_destruction {

typedef int                                 arg_type;
typedef ma::handler_storage<arg_type>       handler_storage_type;
typedef MA_SHARED_PTR<handler_storage_type> handler_storage_ptr;

class testable_handler_storage : public handler_storage_type
{
public:
  testable_handler_storage(boost::asio::io_service& io_service,
      std::size_t& counter)
    : handler_storage_type(io_service)
    , counter_(counter)
  {
    ++counter_;
  }

  ~testable_handler_storage()
  {
    --counter_;
  }

private:
  std::size_t& counter_;
}; // testable_handler_storage

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

class simple_handler
{
private:
  typedef simple_handler this_type;

public:
  explicit simple_handler(std::size_t& counter)
    : counter_(counter)
  {
    ++counter_;
  }

  ~simple_handler()
  {
    --counter_;
  }

  simple_handler(const this_type& other)
    : counter_(other.counter_)
  {
    ++counter_;
  }

#if defined(MA_HAS_RVALUE_REFS)
  simple_handler(this_type&& other)
    : counter_(other.counter_)
  {
    ++counter_;
  }
#endif

  void operator()(const arg_type&)
  {
  }

private:
  std::size_t& counter_;
}; // class simple_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

class hooked_handler
{
private:
  typedef hooked_handler this_type;

public:
  hooked_handler(const handler_storage_ptr& handler_storage,
      std::size_t& counter)
    : handler_storage_(handler_storage)
    , counter_(counter)
  {
    ++counter_;
  }

  ~hooked_handler()
  {
    --counter_;
  }

  hooked_handler(const this_type& other)
    : handler_storage_(other.handler_storage_)
    , counter_(other.counter_)
  {
    ++counter_;
  }

#if defined(MA_HAS_RVALUE_REFS)
  hooked_handler(this_type&& other)
    : handler_storage_(std::move(other.handler_storage_))
    , counter_(other.counter_)
  {
    ++counter_;
  }
#endif

  void operator()(const arg_type&)
  {
  }

private:
  handler_storage_ptr handler_storage_;
  std::size_t& counter_;
}; // class hooked_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

class active_destructing_handler
{
private:
  typedef active_destructing_handler this_type;

public:
  typedef MA_FUNCTION<void (void)> continuation;

  active_destructing_handler(const continuation& cont, std::size_t& counter)
    : cont_(MA_MAKE_SHARED<continuation_holder>(cont))
    , counter_(counter)
  {
    ++counter_;
  }

  ~active_destructing_handler()
  {
    --counter_;
  }

  active_destructing_handler(const this_type& other)
    : cont_(other.cont_)
    , counter_(other.counter_)
  {
    ++counter_;
  }

#if defined(MA_HAS_RVALUE_REFS)
  active_destructing_handler(this_type&& other)
    : cont_(std::move(other.cont_))
    , counter_(other.counter_)
  {
    ++counter_;
  }
#endif

  void operator()(const arg_type&)
  {
  }

private:
  class continuation_holder : private boost::noncopyable
  {
  public:
    explicit continuation_holder(const continuation& cont)
      : cont_(cont)
    {
    }

    ~continuation_holder()
    {
       cont_();
    }
  private:
    continuation cont_;
  }; // continuation_holder

  typedef MA_SHARED_PTR<continuation_holder> continuation_holder_ptr;

  continuation_holder_ptr cont_;
  std::size_t& counter_;
}; // class active_destructing_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

namespace simple {

void run_test()
{
  std::size_t counter = 0;
  boost::asio::io_service io_service;
  {
    testable_handler_storage handler_storage1(io_service, counter);
    handler_storage1.store(simple_handler(counter));

    testable_handler_storage handler_storage2(io_service, counter);
    handler_storage2.store(simple_handler(counter));
  }
  BOOST_ASSERT_MSG(0 == counter, "Not all objects were destroyed");
}

} // namespace simple

void store_simple_handler(const handler_storage_ptr& storage,
    std::size_t counter)
{
  storage->store(simple_handler(counter));
}

void store_hooked_handler(const handler_storage_ptr& storage,
    const handler_storage_ptr& hooked_storage, std::size_t counter)
{
  storage->store(hooked_handler(hooked_storage, counter));
}

namespace cyclic_references {

void run_test()
{
  std::size_t counter = 0;
  {
    boost::asio::io_service io_service;

    handler_storage_ptr handler_storage1 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage1->store(hooked_handler(handler_storage1, counter));

    handler_storage_ptr handler_storage2 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage2->store(active_destructing_handler(
        MA_BIND(store_hooked_handler, handler_storage2, 
            handler_storage2, MA_REF(counter)),
        counter));
  }
  BOOST_ASSERT_MSG(0 == counter, "Not all objects were destroyed");
}

} // namespace cyclil_references

namespace reenterable_call {

void run_test()
{
  std::size_t counter = 0;
  {
    boost::asio::io_service io_service;

    testable_handler_storage handler_storage1(io_service, counter);
    handler_storage1.store(simple_handler(counter));

    testable_handler_storage handler_storage2(io_service, counter);
    handler_storage2.store(simple_handler(counter));

    handler_storage_ptr handler_storage3 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage3->store(hooked_handler(handler_storage3, counter));

    testable_handler_storage handler_storage4(io_service, counter);
    handler_storage4.store(simple_handler(counter));

    handler_storage_ptr handler_storage5 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage5->store(active_destructing_handler(
        MA_BIND(store_simple_handler, handler_storage5, MA_REF(counter)),
        counter));

    testable_handler_storage handler_storage6(io_service, counter);
    handler_storage6.store(simple_handler(counter));

    handler_storage_ptr handler_storage7 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage7->store(active_destructing_handler(
        MA_BIND(store_hooked_handler, handler_storage7, 
            handler_storage7, MA_REF(counter)),
        counter));

    handler_storage_ptr handler_storage8 =
        MA_MAKE_SHARED<testable_handler_storage>(
            MA_REF(io_service), MA_REF(counter));
    handler_storage8->store(active_destructing_handler(
        MA_BIND(store_hooked_handler, handler_storage7, 
            handler_storage5, MA_REF(counter)),
        counter));
  }
  BOOST_ASSERT_MSG(0 == counter, "Not all objects were destroyed");
}

} // namespace reenterable_call

void run_test()
{
  std::cout << "*** ma::test::handler_storage_service_destruction ***"
      << std::endl;

  simple::run_test();
  cyclic_references::run_test();
  reenterable_call::run_test();
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

static const int value4 = 4;

void run_test()
{
  std::cout << "*** ma::test::handler_storage_target ***" << std::endl;

  std::size_t cpu_count = MA_THREAD::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);

  {
    typedef ma::handler_storage<int, handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(handler(value4));

    BOOST_ASSERT_MSG(value4 == handler_storage.target()->get_value(),
        "Stored value and target are different");
  }
} // run_test

} // namespace handler_storage_target

namespace handler_storage_arg {

typedef MA_FUNCTION<void (void)> continuation;

class test_handler_base
{
private:
  typedef test_handler_base this_type;

public:
  virtual int get_value() const = 0;

protected:
  test_handler_base()
  {
  }

  ~test_handler_base()
  {
  }
}; // class test_handler_base

class void_handler_without_target
{
public:
  void_handler_without_target(int value, const continuation& cont)
    : value_(value)
    , cont_(cont)
  {
  }

  void operator()()
  {
    std::cout << value_ << std::endl;
    cont_();
  }

private:
  int value_;
  continuation cont_;
}; // class void_handler_without_target

class void_handler_with_target : public test_handler_base
{
public:
  void_handler_with_target(int value, const continuation& cont)
    : value_(value)
    , cont_(cont)
  {
  }

  void operator()()
  {
    std::cout << value_ << std::endl;
    cont_();
  }

  int get_value() const
  {
    return value_;
  }

private:
  int value_;
  continuation cont_;
}; // class void_handler_with_target

class int_handler_without_target
{
public:
  int_handler_without_target(int value, const continuation& cont)
    : value_(value)
    , cont_(cont)
  {
  }

  void operator()(int value)
  {
    std::cout << value << " : " << value_ << std::endl;
    cont_();
  }

private:
  int value_;
  continuation cont_;
}; // class int_handler_without_target

class int_handler_with_target : public test_handler_base
{
public:
  int_handler_with_target(int value, const continuation& cont)
    : value_(value)
    , cont_(cont)
  {
  }

  void operator()(int value)
  {
    std::cout << value << " : " << value_ << std::endl;
    cont_();
  }

  int get_value() const
  {
    return value_;
  }

private:
  int value_;
  continuation cont_;
}; // class int_handler_with_target

static const int value4 = 4;
static const int value1 = 1;
static const int value2 = 2;

void run_test()
{
  std::cout << "*** ma::test::handler_storage_arg ***" << std::endl;

  std::size_t cpu_count = MA_THREAD::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);
  ma::detail::latch done_latch;

  {
    typedef ma::handler_storage<void> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_without_target(value4,
        MA_BIND(count_down, MA_REF(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post();
  }

  {
    typedef ma::handler_storage<int> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(int_handler_without_target(value4,
        MA_BIND(count_down, MA_REF(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    typedef ma::handler_storage<void, test_handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_with_target(value4,
        MA_BIND(count_down, MA_REF(done_latch))));

    BOOST_ASSERT_MSG(value4 == handler_storage.target()->get_value(), 
        "Data of target is different than the stored data");

    std::cout << handler_storage.target()->get_value() << std::endl;
    done_latch.count_up();
    handler_storage.post();
  }

  {
    typedef ma::handler_storage<int, test_handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(int_handler_with_target(value4,
        MA_BIND(count_down, MA_REF(done_latch))));

    BOOST_ASSERT_MSG(value4 == handler_storage.target()->get_value(), 
        "Data of target is different than the stored data");

    std::cout << handler_storage.target()->get_value() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    boost::asio::io_service io_service;

    ma::handler_storage<int, test_handler_base> handler_storage1(io_service);
    handler_storage1.store(int_handler_with_target(value1,
        MA_BIND(count_down, MA_REF(done_latch))));

    BOOST_ASSERT_MSG(value1 == handler_storage1.target()->get_value(), 
        "Data of target is different than the stored data");

    ma::handler_storage<void> handler_storage2(io_service);
    handler_storage2.store(void_handler_without_target(value2,
        MA_BIND(count_down, MA_REF(done_latch))));
  }

  done_latch.wait();
}

} // namespace handler_storage_arg

} // namespace test
} // namespace ma
