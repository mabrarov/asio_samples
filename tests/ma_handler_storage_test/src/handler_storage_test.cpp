//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
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
#include <ma/bind_handler.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_storage.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/thread_group.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/utility.hpp>
#include <ma/test/io_service_pool.hpp>

namespace ma {
namespace test {

void count_down(detail::latch& latch)
{
  latch.count_down();
}

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

class simple_handler
{
private:
  typedef simple_handler this_type;

  this_type& operator=(const this_type&);

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

class hooked_handler
{
private:
  typedef hooked_handler this_type;

  this_type& operator=(const this_type&);

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

class active_destructing_handler
{
private:
  typedef active_destructing_handler this_type;

  this_type& operator=(const this_type&);

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
private:
  typedef handler this_type;

  this_type& operator=(const this_type&);

public:
  explicit handler(int& out, int value)
    : out_(out)
    , value_(value)
  {
  }

  void operator()(int value)
  {
    out_ = value;
  }

  void operator()(void)
  {
    out_ = value_;
  }

  int get_value() const
  {
    return value_;
  }

private:
  int& out_;
  int  value_;
}; // class handler

TEST(handler_storage, target_when_empty)
{
  {
    typedef ma::handler_storage<int, handler_base> handler_storage_type;

    boost::asio::io_service io_service;
    handler_storage_type handler_storage(io_service);
    ASSERT_EQ(0, handler_storage.target());
  }
  {
    typedef ma::handler_storage<int> handler_storage_type;

    boost::asio::io_service io_service;
    handler_storage_type handler_storage(io_service);
    ASSERT_EQ(0, handler_storage.target());
  }
} // TEST(handler_storage, target_when_empty)

TEST(handler_storage, target)
{
  typedef ma::handler_storage<int, handler_base> handler_storage_type;

  const int test_value = 42;
  const int test_post_value = 43;
  int out = 0;

  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);
  handler_storage.store(handler(out, test_value));

  ASSERT_EQ(test_value, handler_storage.target()->get_value());

  handler_storage.post(test_post_value);
  io_service.run();

  ASSERT_EQ(test_post_value, out);
} // TEST(handler_storage, target)

} // namespace handler_storage_target

namespace handler_storage_custom_allocation {

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
private:
  typedef handler this_type;

  this_type& operator=(const this_type&);

public:
  explicit handler(int& out, int value)
    : out_(out)
    , value_(value)
  {
  }

  void operator()(int value)
  {
    out_ = value;
  }

  void operator()(void)
  {
    out_ = value_;
  }

private:
  int& out_;
  int  value_;
}; // class handler

class no_default_allocation_handler : public handler
{
private:
  typedef no_default_allocation_handler this_type;
  typedef handler                       base_type;

  this_type& operator=(const this_type&);

public:
  no_default_allocation_handler(int& out, int value)
    : base_type(out, value)
  {
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* /*context*/)
  {
    ADD_FAILURE() << "Custom allocator should be called instead";
    // Forward to default implementation
    return boost::asio::asio_handler_allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* /*context*/)
  {
    ADD_FAILURE() << "Custom allocator should be called instead";
    // Forward to default implementation
    boost::asio::asio_handler_deallocate(pointer, size);
  }
}; // class no_default_allocation_handler

TEST(handler_storage, custom_allocation)
{
  typedef ma::handler_storage<int> handler_storage_type;

  const int stored_value = 42;
  const int test_post_value = 43;
  int out = 0;

  custom_handler_allocator<sizeof(std::size_t) * 16> handler_allocator;
  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);

  handler_storage.store(make_custom_alloc_handler(
      handler_allocator, no_default_allocation_handler(out, stored_value)));
  handler_storage.post(test_post_value);
  io_service.run();

  ASSERT_EQ(handler_allocator.dealloc_count(), handler_allocator.alloc_count());
  ASSERT_GT(handler_allocator.alloc_count(), 0U);
  ASSERT_GT(handler_allocator.dealloc_count(), 0U);
  ASSERT_EQ(test_post_value, out);
} // TEST(handler_storage, custom_allocation)

TEST(handler_storage, custom_allocation_fallback)
{
  typedef ma::handler_storage<int> handler_storage_type;

  const int stored_value = 42;
  const int test_post_value = 43;
  int out = 0;

  custom_handler_allocator<1> handler_allocator;
  boost::asio::io_service io_service;

  handler_storage_type handler_storage(io_service);
  handler_storage.store(make_custom_alloc_handler(
      handler_allocator, handler(out, stored_value)));
  handler_storage.post(test_post_value);
  io_service.run();

  ASSERT_EQ(handler_allocator.alloc_count(), 0U);
  ASSERT_EQ(handler_allocator.dealloc_count(), 0U);
  ASSERT_EQ(test_post_value, out);
} // TEST(handler_storage, custom_allocation_fallback)

TEST(handler_storage, custom_allocation_context_fallback)
{
  typedef ma::handler_storage<int> handler_storage_type;

  const int stored_value = 42;
  const int test_post_value = 43;
  int out = 0;

  custom_handler_allocator<sizeof(std::size_t) * 16> fallback_handler_allocator;
  custom_handler_allocator<1> handler_allocator;
  boost::asio::io_service io_service;

  handler_storage_type handler_storage(io_service);
  handler_storage.store(make_custom_alloc_handler(handler_allocator,
      make_custom_alloc_handler(fallback_handler_allocator,
          no_default_allocation_handler(out, stored_value))));
  handler_storage.post(test_post_value);
  io_service.run();

  ASSERT_EQ(fallback_handler_allocator.dealloc_count(),
      fallback_handler_allocator.alloc_count());
  ASSERT_GT(fallback_handler_allocator.alloc_count(), 0U);
  ASSERT_GT(fallback_handler_allocator.dealloc_count(), 0U);
  ASSERT_EQ(test_post_value, out);
} // TEST(handler_storage, custom_allocation_context_fallback)

} // namespace handler_storage_custom_allocation

namespace handler_storage_post {

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
private:
  typedef void_handler_without_target this_type;

  this_type& operator=(const this_type&);

public:
  void_handler_without_target(int& out, int value, const continuation& cont)
    : out_(out)
    , value_(value)
    , cont_(cont)
  {
  }

  void operator()()
  {
    out_ = value_;
    cont_();
  }

private:
  int&         out_;
  int          value_;
  continuation cont_;
}; // class void_handler_without_target

class void_handler_with_target : public test_handler_base
{
private:
  typedef void_handler_with_target this_type;

  this_type& operator=(const this_type&);

public:
  void_handler_with_target(int& out, int value, const continuation& cont)
    : out_(out)
    , value_(value)
    , cont_(cont)
  {
  }

  void operator()()
  {
    out_ = value_;
    cont_();
  }

  int get_value() const
  {
    return value_;
  }

private:
  int&         out_;
  int          value_;
  continuation cont_;
}; // class void_handler_with_target

class int_handler_without_target
{
private:
  typedef int_handler_without_target this_type;

  this_type& operator=(const this_type&);

public:
  int_handler_without_target(int& out, const continuation& cont)
    : out_(out)
    , cont_(cont)
  {
  }

  void operator()(int value)
  {
    out_ = value;
    cont_();
  }

private:
  int&         out_;
  continuation cont_;
}; // class int_handler_without_target

class int_handler_with_target : public test_handler_base
{
private:
  typedef int_handler_with_target this_type;

  this_type& operator=(const this_type&);

public:
  int_handler_with_target(int& out, int value, const continuation& cont)
    : out_(out)
    , value_(value)
    , cont_(cont)
  {
  }

  void operator()(int value)
  {
    out_ = value;
    cont_();
  }

  int get_value() const
  {
    return value_;
  }

private:
  int&         out_;
  int          value_;
  continuation cont_;
}; // class int_handler_with_target

TEST(handler_storage, post_no_arg)
{
  typedef ma::handler_storage<void> handler_storage_type;

  const int test_value = 42;
  int out = 0;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(
      ma::to_io_context_concurrency_hint(work_thread_count));
  io_service_pool work_threads(io_service, work_thread_count);
  handler_storage_type handler_storage(io_service);
  ma::detail::latch done_latch(1);

  handler_storage.store(void_handler_without_target(out, test_value,
      detail::bind(count_down, detail::ref(done_latch))));

  ASSERT_FALSE(!handler_storage.target());

  handler_storage.post();
  done_latch.wait();

  ASSERT_EQ(test_value, out);
}

TEST(handler_storage, post_no_arg_with_target)
{
  typedef ma::handler_storage<void, test_handler_base> handler_storage_type;

  const int test_value = 42;
  int out = 0;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(
      ma::to_io_context_concurrency_hint(work_thread_count));
  io_service_pool work_threads(io_service, work_thread_count);
  handler_storage_type handler_storage(io_service);
  ma::detail::latch done_latch(1);

  handler_storage.store(void_handler_with_target(out, test_value,
      detail::bind(count_down, detail::ref(done_latch))));

  ASSERT_FALSE(!handler_storage.target());
  ASSERT_EQ(test_value, handler_storage.target()->get_value());

  handler_storage.post();
  done_latch.wait();

  ASSERT_EQ(test_value, out);
}

TEST(handler_storage, post_with_arg)
{
  typedef ma::handler_storage<int> handler_storage_type;

  const int test_value = 43;
  int out = 0;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(
      ma::to_io_context_concurrency_hint(work_thread_count));
  io_service_pool work_threads(io_service, work_thread_count);
  handler_storage_type handler_storage(io_service);
  ma::detail::latch done_latch(1);

  handler_storage.store(int_handler_without_target(out,
      detail::bind(count_down, detail::ref(done_latch))));

  ASSERT_FALSE(!handler_storage.target());

  handler_storage.post(test_value);
  done_latch.wait();

  ASSERT_EQ(test_value, out);
}

TEST(handler_storage, post_with_arg_with_target)
{
  typedef ma::handler_storage<int, test_handler_base> handler_storage_type;

  const int test_value = 43;
  const int test_post_value = 43;
  int out = 0;

  std::size_t cpu_count = detail::thread::hardware_concurrency();
  std::size_t work_thread_count = cpu_count > 1 ? cpu_count : 2;
  boost::asio::io_service io_service(
      ma::to_io_context_concurrency_hint(work_thread_count));
  io_service_pool work_threads(io_service, work_thread_count);
  handler_storage_type handler_storage(io_service);
  ma::detail::latch done_latch(1);

  handler_storage.store(int_handler_with_target(out, test_value,
      detail::bind(count_down, detail::ref(done_latch))));

  ASSERT_FALSE(!handler_storage.target());
  ASSERT_EQ(test_value, handler_storage.target()->get_value());

  handler_storage.post(test_post_value);
  done_latch.wait();

  ASSERT_EQ(test_post_value, out);
} // TEST(handler_storage, arg)

TEST(handler_storage, post_no_arg_when_empty)
{
  typedef ma::handler_storage<void> handler_storage_type;
  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);
  ASSERT_THROW(handler_storage.post(), ma::bad_handler_call);
} // TEST(handler_storage, post_no_arg_when_empty)

TEST(handler_storage, post_no_arg_with_target_when_empty)
{
  typedef ma::handler_storage<void, test_handler_base> handler_storage_type;
  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);
  ASSERT_THROW(handler_storage.post(), ma::bad_handler_call);
} // TEST(handler_storage, post_no_arg_with_target_when_empty)

TEST(handler_storage, post_with_arg_when_empty)
{
  typedef ma::handler_storage<int> handler_storage_type;
  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);
  ASSERT_THROW(handler_storage.post(42), ma::bad_handler_call);
} // TEST(handler_storage, post_with_arg_when_empty)

TEST(handler_storage, post_with_arg_with_target_when_empty)
{
  typedef ma::handler_storage<int, test_handler_base> handler_storage_type;
  boost::asio::io_service io_service;
  handler_storage_type handler_storage(io_service);
  ASSERT_THROW(handler_storage.post(42), ma::bad_handler_call);
} // TEST(handler_storage, post_with_arg_with_target_when_empty)

} // namespace handler_storage_post

namespace handler_storage_move_support {

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

  boost::asio::io_service io_service(ma::to_io_context_concurrency_hint(1));
  io_service_pool work_threads(io_service, 1);

  ma::handler_storage<void> test_handler_storage(io_service);

  std::cout << "*** Create and store handler ***\n";
  test_handler_storage.store(test_handler(done_latch, copy_latch));
  std::cout << "Copy ctr is called (times): "
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS) && (BOOST_ASIO_VERSION >= 101001) \
  && defined(BOOST_ASIO_HAS_MOVE)
  ASSERT_EQ(0U, copy_latch.value());
#endif

  copy_latch.reset();
  std::cout << "*** Post handler ***\n";
  test_handler_storage.post();
  done_latch.wait();

  std::cout << "Copy ctr is called (times): "
      << copy_latch.value() << std::endl;

#if defined(MA_HAS_RVALUE_REFS) && (BOOST_ASIO_VERSION >= 101001) \
  && defined(BOOST_ASIO_HAS_MOVE)
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

#if defined(MA_HAS_RVALUE_REFS) && (BOOST_ASIO_VERSION >= 101001) \
  && defined(BOOST_ASIO_HAS_MOVE)
  ASSERT_EQ(0U, copy_latch.value());
#endif
} // TEST(handler_storage, move_support)

} // namespace handler_storage_move_support

namespace handler_storage_misc {

class trackable
{
private:
  typedef trackable this_type;

  this_type& operator=(const this_type&);

public:
  typedef ma::detail::latch counter_type;

  explicit trackable(counter_type& instance_counter)
    : instance_counter_(instance_counter)
  {
    instance_counter_.count_up();
  }

  trackable(const this_type& other)
    : instance_counter_(other.instance_counter_)
  {
    instance_counter_.count_up();
  }

#if defined(MA_HAS_RVALUE_REFS)
  trackable(this_type&& other)
    : instance_counter_(other.instance_counter_)
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
}; // class trackable

class test_handler : public trackable
{
private:
  typedef test_handler this_type;
  typedef trackable    base_type;

  this_type& operator=(const this_type&);

public:
  explicit test_handler(counter_type& instance_counter)
    : base_type(instance_counter)
  {
  }

  test_handler(const this_type& other)
    : base_type(other)
  {
  }

#if defined(MA_HAS_RVALUE_REFS)
  test_handler(this_type&& other)
    : base_type(detail::move(other))
  {
  }
#endif

  void operator()()
  {
  }

  void operator()(int)
  {
  }
};

TEST(handler_storage, empty_handler_with_void)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<void> handler_storage(io_service);
  ASSERT_TRUE(handler_storage.empty());
  handler_storage.store(test_handler(instance_latch));
  ASSERT_EQ(1U, instance_latch.value());
  ASSERT_FALSE(handler_storage.empty());
} // TEST(handler_storage, empty_handler_with_void)

TEST(handler_storage, has_target_handler_with_void)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<void> handler_storage(io_service);
  handler_storage.store(test_handler(instance_latch));
  ASSERT_EQ(1U, instance_latch.value());
  ASSERT_TRUE(handler_storage.has_target());
} // TEST(handler_storage, has_target_handler_with_void)

TEST(handler_storage, clear_handler_with_void)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<void> handler_storage(io_service);
  handler_storage.store(test_handler(instance_latch));
  handler_storage.clear();
  ASSERT_EQ(0U, instance_latch.value());
  ASSERT_FALSE(handler_storage.has_target());
  ASSERT_TRUE(handler_storage.empty());
} // TEST(handler_storage, clear_handler_with_void)

TEST(handler_storage, empty_handler_with_param)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<int> handler_storage(io_service);
  ASSERT_TRUE(handler_storage.empty());
  handler_storage.store(test_handler(instance_latch));
  ASSERT_EQ(1U, instance_latch.value());
  ASSERT_FALSE(handler_storage.empty());
} // TEST(handler_storage, empty_handler_with_param)

TEST(handler_storage, has_target_handler_with_param)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<int> handler_storage(io_service);
  handler_storage.store(test_handler(instance_latch));
  ASSERT_EQ(1U, instance_latch.value());
  ASSERT_TRUE(handler_storage.has_target());
} // TEST(handler_storage, has_target_handler_with_param)

TEST(handler_storage, clear_handler_with_param)
{
  ma::detail::latch instance_latch;
  boost::asio::io_service io_service;
  ma::handler_storage<int> handler_storage(io_service);
  handler_storage.store(test_handler(instance_latch));
  handler_storage.clear();
  ASSERT_EQ(0U, instance_latch.value());
  ASSERT_FALSE(handler_storage.has_target());
  ASSERT_TRUE(handler_storage.empty());
} // TEST(handler_storage, clear_handler_with_param)

} // namespace handler_storage_misc

} // namespace test
} // namespace ma
