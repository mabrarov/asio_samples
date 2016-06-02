//
// Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>
#include <ma/handler_allocator.hpp>

namespace ma {
namespace test {
namespace handler_allocator {

template <typename Allocator>
class alloc_guard : private boost::noncopyable
{
public:
  typedef Allocator allocator_type;

  alloc_guard(void* ptr, allocator_type& allocator)
    : ptr_(ptr)
    , allocator_(allocator)
  {
  }

  ~alloc_guard()
  {
    allocator_.deallocate(ptr_);
  }

private:
  void* ptr_;
  allocator_type& allocator_;
}; // class alloc_guard

template <typename Allocator>
void test_max_size_allocation(Allocator& allocator)
{
  void* ptr = allocator.allocate(allocator.size());
  alloc_guard<Allocator> guard(ptr, allocator);
  ASSERT_NE(static_cast<void*>(0), ptr);
  (void) guard;
}

template <typename Allocator>
void test_min_size_allocation(Allocator& allocator)
{
  void* ptr = allocator.allocate(1);
  alloc_guard<Allocator> guard(ptr, allocator);
  ASSERT_NE(static_cast<void*>(0), ptr);
  (void) guard;
}

template <typename Allocator>
void test_failed_allocation(Allocator& allocator)
{
  void* ptr = allocator.allocate(allocator.size() + 1);
  alloc_guard<Allocator> guard(ptr, allocator);
  ASSERT_EQ(static_cast<void*>(0), ptr);
  (void) guard;
}

TEST(in_place_handler_allocator, allocation_of_max_size)
{
  in_place_handler_allocator<32> allocator;
  test_max_size_allocation(allocator);
}

TEST(in_place_handler_allocator, allocation_of_min_size)
{
  in_place_handler_allocator<32> allocator;
  test_min_size_allocation(allocator);
}

TEST(in_place_handler_allocator, failed_allocation)
{
  in_place_handler_allocator<32> allocator;
  test_failed_allocation(allocator);
}

TEST(in_heap_handler_allocator, allocation_of_max_size)
{
  in_heap_handler_allocator allocator(128, false);
  test_max_size_allocation(allocator);
}

TEST(in_heap_handler_allocator, allocation_of_min_size)
{
  in_heap_handler_allocator allocator(128, false);
  test_min_size_allocation(allocator);
}

TEST(in_heap_handler_allocator, failed_allocation)
{
  in_heap_handler_allocator allocator(128, false);
  test_failed_allocation(allocator);
}

TEST(lazy_in_heap_handler_allocator, allocation_of_max_size)
{
  in_heap_handler_allocator allocator(16, true);
  test_max_size_allocation(allocator);
}

TEST(lazy_in_heap_handler_allocator, allocation_of_min_size)
{
  in_heap_handler_allocator allocator(16, true);
  test_min_size_allocation(allocator);
}

TEST(lazy_in_heap_handler_allocator, failed_allocation)
{
  in_heap_handler_allocator allocator(16, true);
  test_failed_allocation(allocator);
}

template <typename Allocator>
void test_null_ptr_deallocation(Allocator& allocator)
{
  void* ptr1 = allocator.allocate(allocator.size());
  alloc_guard<Allocator> guard1(ptr1, allocator);
  // Just check that no exception is thrown
  allocator.deallocate(0);
  // Check that deallocation of null ptr doesn't change state of allocator
  void* ptr2 = allocator.allocate(allocator.size());
  alloc_guard<Allocator> guard2(ptr2, allocator);
  ASSERT_EQ(static_cast<void*>(0), ptr2);
  (void) guard2;
  (void) guard1;
}

TEST(in_place_handler_allocator, null_ptr_deallocation)
{
  in_place_handler_allocator<32> allocator;
  test_null_ptr_deallocation(allocator);
}

TEST(in_heap_handler_allocator, null_ptr_deallocation)
{
  in_heap_handler_allocator allocator(16, false);
  test_null_ptr_deallocation(allocator);
}

TEST(lazy_in_heap_handler_allocator, null_ptr_deallocation)
{
  in_heap_handler_allocator allocator(8, true);
  test_null_ptr_deallocation(allocator);
}

template <typename Allocator>
void test_deallocation(Allocator& allocator)
{
  {
    // Ensure that after allocation we can not allocate one more time because
    // allocator is fully loaded
    void* ptr1 = allocator.allocate(allocator.size());
    alloc_guard<Allocator> guard1(ptr1, allocator);
    void* ptr2 = allocator.allocate(allocator.size());
    alloc_guard<Allocator> guard2(ptr2, allocator);
    ASSERT_EQ(static_cast<void*>(0), ptr2);
    (void) guard2;
    (void) guard1;
  }
  {
    // Ensure that after deallocation we can allocate one more time
    void* ptr = allocator.allocate(allocator.size());
    alloc_guard<Allocator> guard(ptr, allocator);
    ASSERT_NE(static_cast<void*>(0), ptr);
    (void) guard;
  }
}

TEST(in_place_handler_allocator, deallocation)
{
  in_place_handler_allocator<32> allocator;
  test_deallocation(allocator);
}

TEST(in_heap_handler_allocator, deallocation)
{
  in_heap_handler_allocator allocator(16, false);
  test_deallocation(allocator);
}

TEST(lazy_in_heap_handler_allocator, deallocation)
{
  in_heap_handler_allocator allocator(8, true);
  test_deallocation(allocator);
}

template<typename Allocator>
void test_ownership_range(Allocator& allocator, void* ptr)
{
  ASSERT_TRUE(allocator.owns(ptr));
  ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) - 1));
  ASSERT_TRUE(allocator.owns(static_cast<char*>(ptr) + 1));
  ASSERT_TRUE(allocator.owns(static_cast<char*>(ptr) + allocator.size() - 1));
  ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) + allocator.size()));
}

TEST(in_place_handler_allocator, ownership_of_null)
{
  in_place_handler_allocator<32> allocator;
  ASSERT_FALSE(allocator.owns(0));
}

TEST(in_place_handler_allocator, ownership_of_allocated)
{
  typedef in_place_handler_allocator<32> allocator_type;
  allocator_type allocator;
  void* ptr = allocator.allocate(allocator.size());
  alloc_guard<allocator_type> guard(ptr, allocator);
  test_ownership_range(allocator, ptr);
  (void) guard;
}

TEST(in_place_handler_allocator, ownership_of_free)
{
  in_place_handler_allocator<32> allocator;
  void* ptr = allocator.allocate(allocator.size());
  allocator.deallocate(ptr);
  test_ownership_range(allocator, ptr);
}

TEST(in_heap_handler_allocator, ownership_of_null)
{
  in_heap_handler_allocator allocator(64, false);
  ASSERT_FALSE(allocator.owns(0));
}

TEST(in_heap_handler_allocator, ownership_of_allocated)
{
  typedef in_heap_handler_allocator allocator_type;
  allocator_type allocator(64, false);
  void* ptr = allocator.allocate(allocator.size());
  alloc_guard<allocator_type> guard(ptr, allocator);
  test_ownership_range(allocator, ptr);
  (void) guard;
}

TEST(in_heap_handler_allocator, ownership_of_free)
{
  in_heap_handler_allocator allocator(64, false);
  void* ptr = allocator.allocate(allocator.size());
  allocator.deallocate(ptr);
  test_ownership_range(allocator, ptr);
}

TEST(lazy_in_heap_handler_allocator, ownership_of_null)
{
  in_heap_handler_allocator allocator(128, true);
  ASSERT_FALSE(allocator.owns(0));
}

TEST(lazy_in_heap_handler_allocator, ownership_of_near_null)
{
  in_heap_handler_allocator allocator(128, true);
  ASSERT_FALSE(allocator.owns(static_cast<char*>(0) + 1));
}

TEST(lazy_in_heap_handler_allocator, ownership_of_allocated)
{
  typedef in_heap_handler_allocator allocator_type;
  allocator_type allocator(128, true);
  void* ptr = allocator.allocate(allocator.size());
  alloc_guard<allocator_type> guard(ptr, allocator);
  test_ownership_range(allocator, ptr);
  (void) guard;
}

TEST(lazy_in_heap_handler_allocator, ownership_of_free)
{
  in_heap_handler_allocator allocator(128, true);
  void* ptr = allocator.allocate(allocator.size());
  allocator.deallocate(ptr);
  test_ownership_range(allocator, ptr);
}

} // namespace handler_allocator
} // namespace test
} // namespace ma
