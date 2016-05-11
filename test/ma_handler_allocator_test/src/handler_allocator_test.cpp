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

template <typename Allocator>
void test_ownership(Allocator& allocator)
{
  // Explicitly check correct handing of null pointer
  ASSERT_FALSE(allocator.owns(0));
  void* ptr = allocator.allocate(allocator.size());
  {
    alloc_guard<Allocator> guard(ptr, allocator);
    // Check ownership of allocated memory block
    ASSERT_TRUE(allocator.owns(ptr));
    ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) - 1));
    ASSERT_TRUE(allocator.owns(static_cast<char*>(ptr) + 1));
    ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) + allocator.size()));
    (void) guard;
  }
  // Check ownership of free memory block
  ASSERT_TRUE(allocator.owns(ptr));
  ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) - 1));
  ASSERT_TRUE(allocator.owns(static_cast<char*>(ptr) + 1));
  ASSERT_FALSE(allocator.owns(static_cast<char*>(ptr) + allocator.size()));
}

TEST(in_place_handler_allocator, ownership)
{
  in_place_handler_allocator<32> allocator;
  test_ownership(allocator);
}

TEST(in_heap_handler_allocator, ownership)
{
  in_heap_handler_allocator allocator(64, false);
  test_ownership(allocator);
}

TEST(lazy_in_heap_handler_allocator, ownership)
{
  in_heap_handler_allocator allocator(128, true);
  test_ownership(allocator);
}

} // namespace handler_allocator_ownership

} // namespace test
} // namespace ma
