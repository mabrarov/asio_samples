//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
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
#include <ma/config.hpp>
#include <ma/detail/utility.hpp>
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
  in_place_handler_allocator() MA_NOEXCEPT;

  /// For debug purposes (ability to check destruction order, etc).
  ~in_place_handler_allocator() MA_NOEXCEPT;

  /// Allocates memory from internal memory block if it is free and is
  /// large enough. Elsewhere returns null pointer.
  void* allocate(std::size_t size) MA_NOEXCEPT;

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer) MA_NOEXCEPT;

  /// Checks if memory block of size 1 is owned by allocator - as allocated or
  /// as free.
  bool owns(void* pointer) const MA_NOEXCEPT;

  /// Returns max size allocator can allocate
  std::size_t size() const MA_NOEXCEPT;

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
private:
  typedef char byte_type;

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
  typedef detail::unique_ptr<byte_type[]> storage_type;
#else
  typedef detail::scoped_array<byte_type> storage_type;
#endif

public:
  explicit in_heap_handler_allocator(std::size_t size, bool lazy = false);

  /// For debug purposes (ability to check destruction order, etc).
  ~in_heap_handler_allocator() MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
      static_cast<storage_type*>(0)->storage_type::~storage_type()));

  /// Allocates memory from internal memory block if it is free and is
  /// large enough. Elsewhere returns null pointer.
  void* allocate(std::size_t size);

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer) MA_NOEXCEPT;

  /// Checks if memory block of size 1 is owned by allocator - as allocated or
  /// as free.
  bool owns(void* pointer) const MA_NOEXCEPT;

  /// Returns max size allocator can allocate
  std::size_t size() const MA_NOEXCEPT;

private:
  static byte_type* allocate_storage(std::size_t);

  storage_type storage_;
  std::size_t  size_;
  bool         in_use_;
}; // class in_heap_handler_allocator

template <std::size_t alloc_size>
in_place_handler_allocator<alloc_size>::in_place_handler_allocator() MA_NOEXCEPT
  : in_use_(false)
{
}

template <std::size_t alloc_size>
in_place_handler_allocator<alloc_size>::~in_place_handler_allocator()
    MA_NOEXCEPT
{
  BOOST_ASSERT_MSG(!in_use_, "Allocator is still used");
}

template <std::size_t alloc_size>
void* in_place_handler_allocator<alloc_size>::allocate(std::size_t size)
    MA_NOEXCEPT
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
    MA_NOEXCEPT
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
    MA_NOEXCEPT
{
  const byte_type* begin = static_cast<const byte_type*>(storage_.address());
  const byte_type* end = begin + alloc_size;
  const byte_type* p = static_cast<const byte_type*>(pointer);
  return (p >= begin) && (p < end);
}

template <std::size_t alloc_size>
std::size_t in_place_handler_allocator<alloc_size>::size() const MA_NOEXCEPT
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
    MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(
        static_cast<storage_type*>(0)->storage_type::~storage_type()))
{
  BOOST_ASSERT_MSG(!in_use_, "Allocator is still used");
}

inline void* in_heap_handler_allocator::allocate(std::size_t size)
    MA_NOEXCEPT_IF(MA_NOEXCEPT_EXPR(allocate_storage(size)))
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

inline void in_heap_handler_allocator::deallocate(void* pointer) MA_NOEXCEPT
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

inline bool in_heap_handler_allocator::owns(void* pointer) const MA_NOEXCEPT
{
  const byte_type* begin = storage_.get();
  const byte_type* end = begin + size_;
  const byte_type* p = static_cast<const byte_type*>(pointer);
  return (p >= begin) && (p < end) && begin && p;
}

inline std::size_t in_heap_handler_allocator::size() const MA_NOEXCEPT
{
  return size_;
}

} // namespace ma

#endif // MA_HANDLER_ALLOCATOR_HPP
