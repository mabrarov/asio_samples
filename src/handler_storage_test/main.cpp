//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
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
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_storage.hpp>
#include <ma/lockable_wrapped_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/thread_group.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/utility.hpp>

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

namespace handler_move_support {

void run_test();

} // namespace handler_move_support

} // namespace test
} // namespace ma

#if defined(MA_WIN32_TMAIN)
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
    ma::test::handler_move_support::run_test();
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

void count_down(detail::latch& latch)
{
  latch.count_down();
}

class io_service_pool : private boost::noncopyable
{
public:
  io_service_pool(boost::asio::io_service& io_service, std::size_t size)
    : work_guard_(boost::in_place(detail::ref(io_service)))
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      threads_.create_thread(detail::bind(
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

  optional_io_work work_guard_;
  ma::thread_group threads_;
}; // class io_service_pool

namespace lockable_wrapper {

typedef std::string data_type;
typedef detail::function<void(void)> continuation;

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
  typedef detail::mutex                  mutex_type;
  typedef detail::lock_guard<mutex_type> lock_guard_type;

  std::cout << "*** ma::test::lockable_wrapper ***" << std::endl;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
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

    io_service.post(ma::make_lockable_wrapped_handler(mutex, detail::bind(
        mutating_func1, detail::ref(data), continuation(
            detail::bind(count_down, detail::ref(done_latch))))));

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

typedef int                                      arg_type;
typedef ma::handler_storage<arg_type>            handler_storage_type;
typedef detail::shared_ptr<handler_storage_type> handler_storage_ptr;

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
    : handler_storage_(detail::move(other.handler_storage_))
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
  typedef detail::function<void(void)> continuation;

  active_destructing_handler(const continuation& cont, std::size_t& counter)
    : cont_(detail::make_shared<continuation_holder>(cont))
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
    : cont_(detail::move(other.cont_))
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

  typedef detail::shared_ptr<continuation_holder> continuation_holder_ptr;

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
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage1->store(hooked_handler(handler_storage1, counter));

    handler_storage_ptr handler_storage2 =
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage2->store(active_destructing_handler(
        detail::bind(store_hooked_handler, handler_storage2,
            handler_storage2, detail::ref(counter)),
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
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage3->store(hooked_handler(handler_storage3, counter));

    testable_handler_storage handler_storage4(io_service, counter);
    handler_storage4.store(simple_handler(counter));

    handler_storage_ptr handler_storage5 =
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage5->store(active_destructing_handler(
        detail::bind(store_simple_handler, handler_storage5, 
            detail::ref(counter)), counter));

    testable_handler_storage handler_storage6(io_service, counter);
    handler_storage6.store(simple_handler(counter));

    handler_storage_ptr handler_storage7 =
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage7->store(active_destructing_handler(
        detail::bind(store_hooked_handler, handler_storage7,
            handler_storage7, detail::ref(counter)),
        counter));

    handler_storage_ptr handler_storage8 =
        detail::make_shared<testable_handler_storage>(
            detail::ref(io_service), detail::ref(counter));
    handler_storage8->store(active_destructing_handler(
        detail::bind(store_hooked_handler, handler_storage7,
            handler_storage5, detail::ref(counter)),
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

  std::size_t cpu_count = detail::thread::hardware_concurrency();
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

typedef detail::function<void(void)> continuation;

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

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);
  ma::detail::latch done_latch;

  {
    typedef ma::handler_storage<void> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_without_target(value4,
        detail::bind(count_down, detail::ref(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post();
  }

  {
    typedef ma::handler_storage<int> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(int_handler_without_target(value4,
        detail::bind(count_down, detail::ref(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    typedef ma::handler_storage<void, test_handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_with_target(value4,
        detail::bind(count_down, detail::ref(done_latch))));

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
        detail::bind(count_down, detail::ref(done_latch))));

    BOOST_ASSERT_MSG(value4 == handler_storage.target()->get_value(), 
        "Data of target is different than the stored data");

    std::cout << handler_storage.target()->get_value() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    boost::asio::io_service another_io_service;

    ma::handler_storage<int, test_handler_base> handler_storage1(
        another_io_service);
    handler_storage1.store(int_handler_with_target(value1,
        detail::bind(count_down, detail::ref(done_latch))));

    BOOST_ASSERT_MSG(value1 == handler_storage1.target()->get_value(), 
        "Data of target is different than the stored data");

    ma::handler_storage<void> handler_storage2(another_io_service);
    handler_storage2.store(void_handler_without_target(value2,
        detail::bind(count_down, detail::ref(done_latch))));
  }

  done_latch.wait();
}

} // namespace handler_storage_arg

namespace handler_move_support {

class trackable
{
private:
  typedef trackable this_type;

  this_type& operator=(const this_type&);

public:
  typedef ma::detail::latch counter_type;

  explicit trackable(counter_type& instance_counter, counter_type& copy_counter)
    : instance_counter_(instance_counter)
    , copy_counter_(copy_counter)
  {
    instance_counter_.count_up();
  }

  trackable(const this_type& other)
    : instance_counter_(other.instance_counter_)
    , copy_counter_(other.copy_counter_)
  {
    instance_counter_.count_up();
    copy_counter_.count_up();
  }

#if defined(MA_HAS_RVALUE_REFS)
  trackable(this_type&& other)
    : instance_counter_(other.instance_counter_)
    , copy_counter_(other.copy_counter_)
  {
    instance_counter_.count_up();
  }
#endif

  ~trackable()
  {
    instance_counter_.count_down();
  }

private:
  counter_type& instance_counter_;
  counter_type& copy_counter_;
}; // class trackable

class test_handler : public trackable
{
private:
  typedef test_handler this_type;
  typedef trackable    base_type;

  this_type& operator=(const this_type&);

public:
  test_handler(counter_type& instance_counter, counter_type& copy_counter)
    : base_type(instance_counter, copy_counter)
  {
    std::cout << "test_handler   : ctr!\n";
  }

  test_handler(const this_type& other)
    : base_type(other)
  {
    std::cout << "test_handler   : copy ctr!\n";
  }

#if defined(MA_HAS_RVALUE_REFS)
  test_handler(this_type&& other)
    : base_type(detail::move(other))
  {
    std::cout << "test_handler   : move ctr\n";
  }
#endif

  ~test_handler()
  {
    std::cout << "test_handler   : dtr\n";
  }

  void operator()()
  {
    std::cout << "test_handler   : operator()\n";
  }
}; // class test_handler

typedef ma::handler_storage<void> handler_storage_type;

class context_handler : public trackable
{
private:
  typedef context_handler this_type;
  typedef trackable       base_type;

  this_type& operator=(const this_type&);

public:
  context_handler(counter_type& instance_counter, counter_type& copy_counter,
      handler_storage_type& storage)
    : base_type(instance_counter, copy_counter)
    , storage_(storage)
  {
    std::cout << "context_handler: ctr!\n";
  }

  context_handler(const this_type& other)
    : base_type(other)
    , storage_(other.storage_)
  {
    std::cout << "context_handler: copy ctr!\n";
  }

#if defined(MA_HAS_RVALUE_REFS)
  context_handler(this_type&& other)
    : base_type(detail::move(other))
    , storage_(other.storage_)
  {
    std::cout << "context_handler: move ctr\n";
  }
#endif

  ~context_handler()
  {
    std::cout << "context_handler: dtr\n";
  }

  template <typename Handler>
  void operator()(Handler& handler)
  {
    std::cout << "context_handler: operator() - store\n";
    storage_.store(detail::move(handler));

    std::cout << "context_handler: operator() - post\n";
    storage_.post();
  }

private:
  handler_storage_type& storage_;
}; // class context_handler

void run_test()
{
  std::cout << "*** ma::test::handler_move_support ***\n";

  ma::detail::latch done_latch;
  ma::detail::latch copy_latch;

  boost::asio::io_service io_service(1);
  io_service_pool work_threads(io_service, 1);

  ma::handler_storage<void> test_handler_storage(io_service);

  std::cout << "*** Create and store handler ***\n";
  test_handler_storage.store(test_handler(done_latch, copy_latch));
  std::cout << "Copy ctr is called (times): " 
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS)
  BOOST_ASSERT_MSG(!copy_latch.value(),
      "Copy ctr of handler should not be called if move ctr exists");
#endif

  std::cout << "*** Post handler ***\n";
  test_handler_storage.post();
  std::cout << "Copy ctr is called (times): " 
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS)
  BOOST_ASSERT_MSG(!copy_latch.value(),
      "Copy ctr of handler should not be called if move ctr exists");
#endif

  done_latch.wait();

  std::cout << "*** Context allocated handler ***\n";
  io_service.post(ma::make_explicit_context_alloc_handler(
      test_handler(done_latch, copy_latch),
      context_handler(done_latch, copy_latch, test_handler_storage)));
  std::cout << "Copy ctr is called (times): " 
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS)
  BOOST_ASSERT_MSG(!copy_latch.value(),
      "Copy ctr of handler should not be called if move ctr exists");
#endif

  done_latch.wait();
}

} // namespace handler_move_support

} // namespace test
} // namespace ma
