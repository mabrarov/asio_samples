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

namespace handler_allocator_ownership {

template<typename Allocator>
void test_ownership_range(Allocator& allocator, void* ptr)
{
  ASSERT_TRUE(allocator.owns(ptr));
  ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) - 1));
  ASSERT_TRUE(allocator.owns(static_cast<char*>(ptr) + 1));
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

} // namespace handler_allocator_ownership

} // namespace test
} // namespace ma
