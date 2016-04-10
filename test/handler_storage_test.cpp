//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <iostream>
#include <limits>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <gtest/gtest.h>
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
#include <ma/test/io_service_pool.hpp>

namespace ma {
namespace test {

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

TEST(handler_storage, destruction_simple)
{
  std::size_t counter = 0;
  boost::asio::io_service io_service;
  {
    testable_handler_storage handler_storage1(io_service, counter);
    handler_storage1.store(simple_handler(counter));

    testable_handler_storage handler_storage2(io_service, counter);
    handler_storage2.store(simple_handler(counter));
  }
  ASSERT_EQ(0U, counter);
} // TEST(handler_storage, destruction_simple)

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

TEST(handler_storage, destruction_cyclic_references)
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
  ASSERT_EQ(0U, counter);
} // TEST(handler_storage, destruction_cyclic_references)

TEST(handler_storage, destruction_reenterable_call)
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
  ASSERT_EQ(0U, counter);
} // TEST(handler_storage, destruction_reenterable_call)

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

TEST(handler_storage, target)
{
  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);

  {
    typedef ma::handler_storage<int, handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(handler(value4));

    ASSERT_EQ(value4, handler_storage.target()->get_value());
  }
} // TEST(handler_storage, target)

} // namespace handler_storage_target

namespace custom_allocation {

template <std::size_t alloc_size>
class custom_handler_allocator : private in_place_handler_allocator<alloc_size>
{
private:
  typedef in_place_handler_allocator<alloc_size> base_type;

public:
  custom_handler_allocator()
    : base_type()
    , alloc_count_(0)
    , dealloc_count_(0)
  {
  }

  void* allocate(std::size_t size)
  {
    void* ptr = base_type::allocate(size);
    if (ptr)
    {
      ++alloc_count_;
    }
    return ptr;
  }


  void deallocate(void* pointer)
  {
    ++dealloc_count_;
    base_type::deallocate(pointer);
  }

  bool owns(void* pointer)
  {
    return base_type::owns(pointer);
  }

  size_t alloc_count() const
  {
    return alloc_count_;
  }

  size_t dealloc_count() const
  {
    return dealloc_count_;
  }

private:
  std::size_t alloc_count_;
  std::size_t dealloc_count_;
}; // class custom_handler_allocator

class handler
{
public:
  explicit handler(int value)
    : value_(value)
  {
  }

  void operator()(int val)
  {
    std::cout << value_ << ' ' << val << std::endl;
  }

  void operator()(void)
  {
    std::cout << value_ << std::endl;
  }

private:
  int value_;
}; // class handler

static const int value = 42;

TEST(handler_storage, custom_allocation)
{
  typedef ma::handler_storage<int> handler_storage_type;

  custom_handler_allocator<sizeof(std::size_t) * 8> handler_allocator;
  boost::asio::io_service io_service;  

  handler_storage_type handler_storage(io_service);
  handler_storage.store(make_custom_alloc_handler(
      handler_allocator, handler(value)));
  handler_storage.post(value + value);
  io_service.run();

  ASSERT_EQ(handler_allocator.dealloc_count(), handler_allocator.alloc_count());
  ASSERT_GT(handler_allocator.alloc_count(), 0U);
  ASSERT_GT(handler_allocator.dealloc_count(), 0U);
} // TEST(handler_storage, custom_allocation)

TEST(handler_storage, custom_allocation_fallback)
{
  typedef ma::handler_storage<int> handler_storage_type;

  custom_handler_allocator<1> handler_allocator;
  boost::asio::io_service io_service;

  handler_storage_type handler_storage(io_service);
  handler_storage.store(make_custom_alloc_handler(
      handler_allocator, handler(value)));
  handler_storage.post(value + value);
  io_service.run();

  ASSERT_EQ(handler_allocator.alloc_count(), 0U);
  ASSERT_EQ(handler_allocator.dealloc_count(), 0U);
} // TEST(handler_storage, custom_allocation_fallback)

TEST(handler_storage, custom_allocation_context_fallback)
{
  typedef ma::handler_storage<int> handler_storage_type;

  custom_handler_allocator<sizeof(std::size_t) * 8> fallback_handler_allocator;
  custom_handler_allocator<1> handler_allocator;
  boost::asio::io_service io_service;

  handler_storage_type handler_storage(io_service);
  handler_storage.store(make_custom_alloc_handler(handler_allocator,
      make_custom_alloc_handler(fallback_handler_allocator, handler(value))));
  handler_storage.post(value + value);
  io_service.run();

  ASSERT_EQ(fallback_handler_allocator.dealloc_count(),
            fallback_handler_allocator.alloc_count());
  ASSERT_GT(fallback_handler_allocator.alloc_count(), 0U);
  ASSERT_GT(fallback_handler_allocator.dealloc_count(), 0U);
} // TEST(handler_storage, custom_allocation_context_fallback)

} // namespace custom_allocation

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

TEST(handler_storage, arg)
{
  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(work_thread_count);
  io_service_pool work_threads(io_service, work_thread_count);
  ma::detail::latch done_latch;

  {
    typedef ma::handler_storage<void> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_without_target(value4,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post();
  }

  {
    typedef ma::handler_storage<int> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(int_handler_without_target(value4,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));

    std::cout << handler_storage.target() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    typedef ma::handler_storage<void, test_handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(void_handler_with_target(value4,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));

    ASSERT_EQ(value4, handler_storage.target()->get_value());

    std::cout << handler_storage.target()->get_value() << std::endl;
    done_latch.count_up();
    handler_storage.post();
  }

  {
    typedef ma::handler_storage<int, test_handler_base> handler_storage_type;

    handler_storage_type handler_storage(io_service);
    handler_storage.store(int_handler_with_target(value4,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));

    ASSERT_EQ(value4, handler_storage.target()->get_value());

    std::cout << handler_storage.target()->get_value() << std::endl;
    done_latch.count_up();
    handler_storage.post(value2);
  }

  {
    boost::asio::io_service another_io_service;

    ma::handler_storage<int, test_handler_base> handler_storage1(
        another_io_service);
    handler_storage1.store(int_handler_with_target(value1,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));

    ASSERT_EQ(value1, handler_storage1.target()->get_value());

    ma::handler_storage<void> handler_storage2(another_io_service);
    handler_storage2.store(void_handler_without_target(value2,
        detail::bind(&detail::latch::count_down, detail::ref(done_latch))));
  }

  done_latch.wait();
} // TEST(handler_storage, arg)

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

TEST(handler_storage, move_support)
{
  ma::detail::latch done_latch;
  ma::detail::latch copy_latch;

  boost::asio::io_service io_service(1);
  io_service_pool work_threads(io_service, 1);

  ma::handler_storage<void> test_handler_storage(io_service);

  std::cout << "*** Create and store handler ***\n";
  test_handler_storage.store(test_handler(done_latch, copy_latch));
  std::cout << "Copy ctr is called (times): "
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS) && defined(BOOST_ASIO_HAS_MOVE)
  ASSERT_EQ(0U, copy_latch.value());
#endif

  copy_latch.reset();
  std::cout << "*** Post handler ***\n";
  test_handler_storage.post();
  done_latch.wait();

  std::cout << "Copy ctr is called (times): "
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS) && defined(BOOST_ASIO_HAS_MOVE)
  ASSERT_EQ(0U, copy_latch.value());
#endif

  copy_latch.reset();
  std::cout << "*** Context allocated handler ***\n";
  io_service.post(ma::make_explicit_context_alloc_handler(
      test_handler(done_latch, copy_latch),
      context_handler(done_latch, copy_latch, test_handler_storage)));
  done_latch.wait();

  std::cout << "Copy ctr is called (times): "
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS) && defined(BOOST_ASIO_HAS_MOVE)
  ASSERT_EQ(0U, copy_latch.value());
#endif
} // TEST(handler_storage, move_support)

} // namespace handler_move_support

} // namespace test
} // namespace ma
