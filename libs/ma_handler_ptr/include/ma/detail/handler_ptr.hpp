//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_HANDLER_PTR_HPP
#define MA_DETAIL_HANDLER_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/utility.hpp>

namespace ma {
namespace detail {

/**
 * Below listed code is a modified copy of Asio sources. Those sources were
 * replaced in last versions of Asio. But IMHO below suggested way is more
 * clear than the last versions of Asio use.
 */

/// Traits for handler allocation.
template <typename AllocContext, typename Value>
struct handler_alloc_traits
{
  typedef AllocContext alloc_context_type;
  typedef Value        value_type;
  typedef Value*       pointer_type;
  BOOST_STATIC_CONSTANT(std::size_t, value_size = sizeof(Value));
}; // struct handler_alloc_traits

template <typename Alloc_Traits>
class handler_ptr;

/// Helper class to provide RAII on uninitialized handler memory.
template <typename Alloc_Traits>
class raw_handler_ptr : private boost::noncopyable
{
public:
  typedef typename Alloc_Traits::alloc_context_type alloc_context_type;
  typedef typename Alloc_Traits::value_type         value_type;
  typedef typename Alloc_Traits::pointer_type       pointer_type;
  BOOST_STATIC_CONSTANT(std::size_t, value_size = Alloc_Traits::value_size);

  /// Constructor that allocates the memory. Can throw.
  raw_handler_ptr(alloc_context_type& alloc_context)
    : alloc_context_(alloc_context)
    , pointer_(static_cast<pointer_type>(
          ma_handler_alloc_helpers::allocate(value_size, alloc_context)))
  {
  }

  /// Steal constructor.
  raw_handler_ptr(alloc_context_type& alloc_context,
      pointer_type pointer) MA_NOEXCEPT
    : alloc_context_(alloc_context)
    , pointer_(pointer)
  {
  }

  /// Destructor that automatically deallocates memory, unless it has been
  /// stolen by a handler_ptr object.
  /// Throws if associated deallocate throws.
  ~raw_handler_ptr()
  {
    if (pointer_)
    {
      ma_handler_alloc_helpers::deallocate(
          pointer_, value_size, alloc_context_);
    }
  }

private:
  friend class handler_ptr<Alloc_Traits>;

  alloc_context_type& alloc_context_;
  pointer_type        pointer_;
}; // raw_handler_ptr

/// Helper class to provide RAII on uninitialized handler memory.
template <typename Alloc_Traits>
class handler_ptr : private boost::noncopyable
{
public:
  typedef raw_handler_ptr<Alloc_Traits>             raw_ptr_type;
  typedef typename Alloc_Traits::alloc_context_type alloc_context_type;
  typedef typename Alloc_Traits::value_type         value_type;
  typedef typename Alloc_Traits::pointer_type       pointer_type;
  BOOST_STATIC_CONSTANT(std::size_t, value_size = Alloc_Traits::value_size);

  /// Take ownership of existing memory.
  handler_ptr(alloc_context_type& alloc_context,
      pointer_type pointer) MA_NOEXCEPT
    : alloc_context_(detail::addressof(alloc_context))
    , pointer_(pointer)
  {
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds.
  /// Throws if constructor of value_type throws.
  handler_ptr(raw_ptr_type& raw_ptr)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type)
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3, MA_FWD_REF(Arg4) a4)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3),
          detail::forward<Arg4>(a4)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3, MA_FWD_REF(Arg4) a4, MA_FWD_REF(Arg5) a5)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3),
          detail::forward<Arg4>(a4), detail::forward<Arg5>(a5)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5, typename Arg6>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3, MA_FWD_REF(Arg4) a4, MA_FWD_REF(Arg5) a5,
      MA_FWD_REF(Arg6) a6)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3),
          detail::forward<Arg4>(a4), detail::forward<Arg5>(a5),
          detail::forward<Arg6>(a6)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5, typename Arg6, typename Arg7>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3, MA_FWD_REF(Arg4) a4, MA_FWD_REF(Arg5) a5,
      MA_FWD_REF(Arg6) a6, MA_FWD_REF(Arg7) a7)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3),
          detail::forward<Arg4>(a4), detail::forward<Arg5>(a5),
          detail::forward<Arg6>(a6), detail::forward<Arg7>(a7)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Construct object in raw memory and take ownership if construction
  /// succeeds. Forwards arg1,..., argn to the object's constructor.
  /// Throws if constructor of value_type throws.
  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4,
      typename Arg5, typename Arg6, typename Arg7, typename Arg8>
  handler_ptr(raw_ptr_type& raw_ptr, MA_FWD_REF(Arg1) a1, MA_FWD_REF(Arg2) a2,
      MA_FWD_REF(Arg3) a3, MA_FWD_REF(Arg4) a4, MA_FWD_REF(Arg5) a5,
      MA_FWD_REF(Arg6) a6, MA_FWD_REF(Arg7) a7, MA_FWD_REF(Arg8) a8)
    : alloc_context_(detail::addressof(raw_ptr.alloc_context_))
    , pointer_(new (raw_ptr.pointer_) value_type(detail::forward<Arg1>(a1),
          detail::forward<Arg2>(a2), detail::forward<Arg3>(a3),
          detail::forward<Arg4>(a4), detail::forward<Arg5>(a5),
          detail::forward<Arg6>(a6), detail::forward<Arg7>(a7),
          detail::forward<Arg8>(a8)))
  {
    raw_ptr.pointer_ = 0;
  }

  /// Destructor automatically deallocates memory, unless it has been released.
  /// Throws if value_type destructor throws.
  /// Throws if associated to alloc_context_type deallocate throws.
  ~handler_ptr()
  {
    reset();
  }

  /// Get the memory.
  pointer_type get() const MA_NOEXCEPT
  {
    return pointer_;
  }

  /// Change allocation context used for memory deallocation. Never throws.
  void set_alloc_context(alloc_context_type& alloc_context) MA_NOEXCEPT
  {
    alloc_context_ = detail::addressof(alloc_context);
  }

  /// Release ownership of the memory.
  pointer_type release() MA_NOEXCEPT
  {
    pointer_type tmp = pointer_;
    pointer_ = 0;
    return tmp;
  }

  /// Explicitly destroy object and deallocate memory.
  /// Throws if value_type destructor throws.
  /// Throws if associated to alloc_context_type deallocate throws.
  void reset()
  {
    if (pointer_)
    {
      // Move memory ownership to guard
      raw_ptr_type raw_ptr(*alloc_context_, pointer_);
      // Zero stored pointer with saving its value to a temporary
      pointer_type tmp = pointer_;
      pointer_ = 0;
      // Destroy stored value
      tmp->value_type::~value_type();
      // Free memory by means of created guard
    }
  }

private:
  alloc_context_type* alloc_context_;
  pointer_type        pointer_;
}; // class handler_ptr

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_HANDLER_PTR_HPP
