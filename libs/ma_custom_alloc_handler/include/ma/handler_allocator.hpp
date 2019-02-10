//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOCATOR_HPP
#define MA_HANDLER_ALLOCATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include <ma/detail/memory.hpp>

namespace ma {

/// Handler allocator to use with ma::custom_alloc_handler.
/// in_place_handler_allocator is based on static size memory block located at
/// in_place_handler_allocator itself. The size of in_place_handler_allocator
/// is part of in_place_handler_allocator type signature.
template <std::size_t alloc_size>
class in_place_handler_allocator : private boost::noncopyable
{
public:
  in_place_handler_allocator();

  /// For debug purposes (ability to check destruction order, etc).
  ~in_place_handler_allocator();

  /// Allocates memory from internal memory block if it is free and is
  /// large enough. Elsewhere returns null pointer.
  void* allocate(std::size_t size);

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer);

  /// Checks if memory block of size 1 is owned by allocator - as allocated or
  /// as free.
  bool owns(void* pointer) const;

  /// Returns max size allocator can allocate
  std::size_t size() const;

private:
  typedef char byte_type;

  boost::aligned_storage<alloc_size> storage_;
  bool in_use_;
}; // class in_place_handler_allocator

/// Handler allocator to use with ma::custom_alloc_handler.
/// in_heap_handler_allocator is based on static size memory block located at
/// heap. The size of in_heap_handler_allocator is defined during construction.
/*
 * Lazy initialization supported.
 */
class in_heap_handler_allocator : private boost::noncopyable
{
public:
  explicit in_heap_handler_allocator(std::size_t size, bool lazy = false);

  /// For debug purposes (ability to check destruction order, etc).
  ~in_heap_handler_allocator();

  /// Allocates memory from internal memory block if it is free and is
  /// large enough. Elsewhere returns null pointer.
  void* allocate(std::size_t size);

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer);

  /// Checks if memory block of size 1 is owned by allocator - as allocated or
  /// as free.
  bool owns(void* pointer) const;

  /// Returns max size allocator can allocate
  std::size_t size() const;

private:
  typedef char byte_type;

  static byte_type* allocate_storage(std::size_t);

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
  detail::unique_ptr<byte_type[]> storage_;
#else
  detail::scoped_array<byte_type> storage_;
#endif

  std::size_t size_;
  bool        in_use_;
}; // class in_heap_handler_allocator

template <std::size_t alloc_size>
in_place_handler_allocator<alloc_size>::in_place_handler_allocator()
  : in_use_(false)
{
}

template <std::size_t alloc_size>
in_place_handler_allocator<alloc_size>::~in_place_handler_allocator()
{
  BOOST_ASSERT_MSG(!in_use_, "Allocator is still used");
}

template <std::size_t alloc_size>
void* in_place_handler_allocator<alloc_size>::allocate(std::size_t size)
{
  if (in_use_ || (size > storage_.size))
  {
    return 0;
  }
  in_use_ = true;
  return storage_.address();
}

template <std::size_t alloc_size>
void in_place_handler_allocator<alloc_size>::deallocate(void* pointer)
{
  BOOST_ASSERT_MSG(
      !pointer || in_use_, "Allocator wasn't marked as used");
  BOOST_ASSERT_MSG(
      !pointer || owns(pointer), "Pointer is not owned by this allocator");
  if (pointer)
  {
    in_use_ = false;
  }
}

template <std::size_t alloc_size>
bool in_place_handler_allocator<alloc_size>::owns(void* pointer) const
{
  const byte_type* begin = static_cast<const byte_type*>(storage_.address());
  const byte_type* end = begin + alloc_size;
  const byte_type* p = static_cast<const byte_type*>(pointer);
  //fixme: Undefined behavior - comparison of unrelated pointers
  // thanks to https://www.viva64.com/en/b/0576/
  return (p >= begin) && (p < end);
}

template <std::size_t alloc_size>
std::size_t in_place_handler_allocator<alloc_size>::size() const
{
  return alloc_size;
}

inline in_heap_handler_allocator::byte_type*
in_heap_handler_allocator::allocate_storage(std::size_t size)
{
  return new byte_type[size];
}

inline in_heap_handler_allocator::in_heap_handler_allocator(
    std::size_t size, bool lazy)
  : storage_(lazy ? 0 : allocate_storage(size))
  , size_(size)
  , in_use_(false)
{
}

inline in_heap_handler_allocator::~in_heap_handler_allocator()
{
  BOOST_ASSERT_MSG(!in_use_, "Allocator is still used");
}

inline void* in_heap_handler_allocator::allocate(std::size_t size)
{
  if (in_use_ || (size > size_))
  {
    return 0;
  }
  if (!storage_.get())
  {
    storage_.reset(allocate_storage(size_));
  }
  in_use_ = true;
  return storage_.get();
}

inline void in_heap_handler_allocator::deallocate(void* pointer)
{
  BOOST_ASSERT_MSG(
      !pointer || in_use_, "Allocator wasn't marked as used");
  BOOST_ASSERT_MSG(
      !pointer || owns(pointer), "Pointer is not owned by this allocator");
  if (pointer)
  {
    in_use_ = false;
  }
}

inline bool in_heap_handler_allocator::owns(void* pointer) const
{
  const byte_type* begin = storage_.get();
  const byte_type* end = begin + size_;
  const byte_type* p = static_cast<const byte_type*>(pointer);
  //fixme: Undefined behavior - comparison of unrelated pointers
  // thanks to https://www.viva64.com/en/b/0576/
  return (p >= begin) && (p < end) && begin && p;
}

inline std::size_t in_heap_handler_allocator::size() const
{
  return size_;
}

} // namespace ma

#endif // MA_HANDLER_ALLOCATOR_HPP
