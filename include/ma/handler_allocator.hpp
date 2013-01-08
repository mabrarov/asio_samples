//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/scoped_array.hpp>

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

  /// Try to allocate memory from internal memory block if it is free and is
  /// large enough. Elsewhere allocate memory by means of global operator new.
  void* allocate(std::size_t size);

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer);

private:
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

  /// Try to allocate memory from internal memory block if it is free and is
  /// large enough. Elsewhere allocate memory by means of global operator new.
  void* allocate(std::size_t size);

  /// Deallocate memory which had previously been allocated by usage of
  /// allocate method.
  void deallocate(void* pointer);

private:
  typedef char byte_type;

  static byte_type* allocate_storage(std::size_t);
  bool storage_initialized() const;
  byte_type* retrieve_aligned_address();

  boost::scoped_array<byte_type> storage_;
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
  if (!in_use_ && (size <= storage_.size))
  {
    in_use_ = true;
    return storage_.address();
  }
  return ::operator new(size);
}

template <std::size_t alloc_size>
void in_place_handler_allocator<alloc_size>::deallocate(void* pointer)
{
  if (storage_.address() == pointer)
  {
    BOOST_ASSERT_MSG(in_use_, "Allocator wasn't marked as used");

    in_use_ = false;
    return;
  }
  ::operator delete(pointer);
}

inline in_heap_handler_allocator::byte_type*
in_heap_handler_allocator::allocate_storage(std::size_t size)
{
  return new byte_type[size];
}

inline bool in_heap_handler_allocator::storage_initialized() const
{
  return 0 != storage_.get();
}

inline in_heap_handler_allocator::byte_type*
in_heap_handler_allocator::retrieve_aligned_address()
{
  if (!storage_.get())
  {
    storage_.reset(allocate_storage(size_));
  }
  return storage_.get();
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
  if (!in_use_ && (size <= size_))
  {
    in_use_ = true;
    return retrieve_aligned_address();
  }
  return ::operator new(size);
}

inline void in_heap_handler_allocator::deallocate(void* pointer)
{
  if (storage_initialized())
  {
    if (retrieve_aligned_address() == pointer)
    {
      BOOST_ASSERT_MSG(in_use_, "Allocator wasn't marked as used");

      in_use_ = false;
      return;
    }
  }
  ::operator delete(pointer);
}

} // namespace ma

#endif // MA_HANDLER_ALLOCATOR_HPP
