//
// Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {
namespace custom_alloc_handler {

template <typename Context>
class alloc_guard : private boost::noncopyable
{
public:
  typedef Context context_type;

  alloc_guard(void* ptr, std::size_t size, context_type& context)
    : ptr_(ptr)
    , size_(size)
    , context_(context)
  {
  }

  ~alloc_guard()
  {
    ma_handler_alloc_helpers::deallocate(ptr_, size_, context_);
  }

private:
  void* ptr_;
  std::size_t size_;
  context_type& context_;
}; // class alloc_guard

template <typename UnderlyingAllocator>
class tracking_allocator : private boost::noncopyable
{
private:
  typedef tracking_allocator<UnderlyingAllocator> this_type;
  typedef UnderlyingAllocator allocator_type;

public:
  explicit tracking_allocator(allocator_type& underlying_allocator,
      detail::latch& counter)
    : underlying_allocator_(underlying_allocator)
    , alloc_counter_(counter)
    , dealloc_counter_(counter)
  {
  }

  tracking_allocator(allocator_type& underlying_allocator,
      detail::latch& alloc_counter, detail::latch& dealloc_counter)
    : underlying_allocator_(underlying_allocator)
    , alloc_counter_(alloc_counter)
    , dealloc_counter_(dealloc_counter)
  {
  }

  void* allocate(std::size_t size)
  {
    alloc_counter_.count_up();
    return underlying_allocator_.allocate(size);
  }

  void deallocate(void* pointer)
  {
    dealloc_counter_.count_up();
    underlying_allocator_.deallocate(pointer);
  }

  bool owns(void* pointer) const
  {
    return underlying_allocator_.owns(pointer);
  }

  std::size_t size() const
  {
    return underlying_allocator_.size();
  }

private:
  allocator_type& underlying_allocator_;
  detail::latch& alloc_counter_;
  detail::latch& dealloc_counter_;
}; // class tracking_allocator

class no_default_allocation_handler
{
private:
  typedef no_default_allocation_handler this_type;

  this_type& operator=(const this_type&);

public:
  void operator()() {}

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

template <typename Handler>
void test_allocation_succeeded(Handler handler, std::size_t size)
{
  void* ptr = ma_handler_alloc_helpers::allocate(size, handler);
  alloc_guard<Handler> guard(ptr, size, handler);
  ASSERT_NE(static_cast<void*>(0), ptr);
  (void) guard;
}

TEST(custom_alloc_handler, allocator_is_used)
{
  typedef in_place_handler_allocator<512> allocator_type;

  allocator_type allocator;
  detail::latch alloc_counter;
  tracking_allocator<allocator_type> tracker(allocator, alloc_counter);
  test_allocation_succeeded(make_custom_alloc_handler(tracker,
      no_default_allocation_handler()), 1);
  ASSERT_EQ(2U, alloc_counter.value());
}

TEST(custom_alloc_handler, io_service_uses_allocator)
{
  typedef in_place_handler_allocator<512> allocator_type;

  allocator_type allocator;
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  tracking_allocator<allocator_type> tracker(allocator, alloc_counter,
      dealloc_counter);
  boost::asio::io_service io_service;
  io_service.post(make_custom_alloc_handler(tracker,
      no_default_allocation_handler()));
  io_service.run();
  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
}

TEST(custom_alloc_handler, in_place_allocator)
{
  in_place_handler_allocator<512> allocator;
  test_allocation_succeeded(
      make_custom_alloc_handler(allocator, no_default_allocation_handler()), 1);
}

TEST(custom_alloc_handler, in_heap_allocator)
{
  in_heap_handler_allocator allocator(512);
  test_allocation_succeeded(
      make_custom_alloc_handler(allocator, no_default_allocation_handler()), 1);
}

class alloc_counting_handler
{
private:
  typedef alloc_counting_handler this_type;

  this_type& operator=(const this_type&);

public:
  explicit alloc_counting_handler(detail::latch& counter)
    : counter_(counter)
  {
  }

  void operator()() {}

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    context->counter_.count_up();
    // Forward to default implementation
    return boost::asio::asio_handler_allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    context->counter_.count_up();
    // Forward to default implementation
    boost::asio::asio_handler_deallocate(pointer, size);
  }

private:
  detail::latch& counter_;
}; // class alloc_counting_handler

TEST(custom_alloc_handler, allocation_fallback)
{
  in_place_handler_allocator<16> allocator;
  detail::latch alloc_counter;
  test_allocation_succeeded(make_custom_alloc_handler(allocator,
      alloc_counting_handler(alloc_counter)), 17);
  ASSERT_EQ(2U, alloc_counter.value());
}

TEST(custom_alloc_handler, success_allocation)
{
  in_place_handler_allocator<512> allocator;
  test_allocation_succeeded(
      make_custom_alloc_handler(allocator, no_default_allocation_handler()), 1);
}

class invoke_counting_handler : public no_default_allocation_handler
{
private:
  typedef invoke_counting_handler this_type;

  this_type& operator=(const this_type&);

public:
  explicit invoke_counting_handler(detail::latch& counter)
    : counter_(counter)
  {
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    context->counter_.count_up();
    boost::asio::asio_handler_invoke(function, context);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    context->counter_.count_up();
    boost::asio::asio_handler_invoke(function, context);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    context->counter_.count_up();
    boost::asio::asio_handler_invoke(function, context);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:
  detail::latch& counter_;
}; // class invoke_counting_handler

template <typename Handler>
void test_allocation_and_invocation(Handler handler, std::size_t size)
{
  {
    void* ptr = ma_handler_alloc_helpers::allocate(size, handler);
    alloc_guard<Handler> guard(ptr, size, handler);
    ASSERT_NE(static_cast<void*>(0), ptr);
    (void) guard;
  }
  ma_handler_invoke_helpers::invoke(handler, handler);
}

TEST(custom_alloc_handler, preserving_invocation_strategy)
{
  detail::latch invoke_counter;
  in_place_handler_allocator<512> allocator;
  test_allocation_and_invocation(make_custom_alloc_handler(allocator,
      invoke_counting_handler(invoke_counter)), 511);
  ASSERT_EQ(1U, invoke_counter.value());
}

} // namespace custom_alloc_handler
} // namespace test
} // namespace ma
