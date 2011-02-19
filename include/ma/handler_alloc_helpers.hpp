//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOC_HELPERS_HPP
#define MA_HANDLER_ALLOC_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/addressof.hpp>
#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma_asio_handler_alloc_helpers
{
  template <typename Context>
  inline void* allocate(std::size_t size, Context& context)
  {
    using namespace boost::asio;
    return asio_handler_allocate(size, boost::addressof(context));
  }

  template <typename Context>
  inline void deallocate(void* pointer, std::size_t size, Context& context)
  {
    using namespace boost::asio;
    asio_handler_deallocate(pointer, size, boost::addressof(context));
  }

} // namespace ma_asio_handler_alloc_helpers

namespace ma
{
  namespace detail
  {
    // Traits for handler allocation.
    template <typename AllocContext, typename Object>
    struct handler_alloc_traits
    {
      typedef AllocContext alloc_context_type;
      typedef Object       value_type;
      typedef Object*      pointer_type;
      BOOST_STATIC_CONSTANT(std::size_t, value_size = sizeof(Object));
    }; // struct handler_alloc_traits

    template <typename Alloc_Traits>
    class handler_ptr;

    // Helper class to provide RAII on uninitialized handler memory.
    template <typename Alloc_Traits>
    class raw_handler_ptr : private boost::noncopyable
    {
    public:
      typedef typename Alloc_Traits::alloc_context_type alloc_context_type;
      typedef typename Alloc_Traits::value_type         value_type;
      typedef typename Alloc_Traits::pointer_type       pointer_type;
      BOOST_STATIC_CONSTANT(std::size_t, value_size = Alloc_Traits::value_size);

      // Constructor allocates the memory
      // Can throw
      raw_handler_ptr(alloc_context_type& alloc_context)
        : alloc_context_(alloc_context)
        , pointer_(static_cast<pointer_type>(ma_asio_handler_alloc_helpers::allocate(value_size, alloc_context)))
      {
      }

      // Steal constructor. Doesn't throw.
      raw_handler_ptr(alloc_context_type& alloc_context, pointer_type pointer)
        : alloc_context_(alloc_context)
        , pointer_(pointer)
      {
      }

      // Destructor automatically deallocates memory, unless it has been stolen by
      // a handler_ptr object.
      ~raw_handler_ptr()
      {
        if (pointer_)
        {
          ma_asio_handler_alloc_helpers::deallocate(pointer_, value_size, alloc_context_);
        }
      }

    private:
      friend class handler_ptr<Alloc_Traits>;
      alloc_context_type& alloc_context_;
      pointer_type pointer_;
    }; // raw_handler_ptr

    // Helper class to provide RAII on uninitialized handler memory.
    template <typename Alloc_Traits>
    class handler_ptr : private boost::noncopyable
    {
    public:
      typedef typename Alloc_Traits::alloc_context_type alloc_context_type;
      typedef typename Alloc_Traits::value_type         value_type;
      typedef typename Alloc_Traits::pointer_type       pointer_type;

      BOOST_STATIC_CONSTANT(std::size_t, value_size = Alloc_Traits::value_size);
      typedef raw_handler_ptr<Alloc_Traits> raw_ptr_type;

      // Take ownership of existing memory.
      handler_ptr(alloc_context_type& alloc_context, pointer_type pointer)
        : alloc_context_(boost::addressof(alloc_context))
        , pointer_(pointer)
      {
      }

      // Construct object in raw memory and 
      // take ownership if construction succeeds.
      handler_ptr(raw_ptr_type& raw_ptr)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type)
      {
        raw_ptr.pointer_ = 0;
      }      

#if defined(MA_HAS_RVALUE_REFS)
      // Construct object in raw memory and 
      // take ownership if construction succeeds.
      template <typename Arg1>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(std::forward<Arg1>(a1)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(std::forward<Arg1>(a1), std::forward<Arg2>(a2)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, Arg3&& a3)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1), std::forward<Arg2>(a2), std::forward<Arg3>(a3)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1), 
            std::forward<Arg2>(a2), 
            std::forward<Arg3>(a3), 
            std::forward<Arg4>(a4)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1), 
            std::forward<Arg2>(a2), 
            std::forward<Arg3>(a3), 
            std::forward<Arg4>(a4), 
            std::forward<Arg5>(a5)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5, Arg6&& a6)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1), 
            std::forward<Arg2>(a2), 
            std::forward<Arg3>(a3), 
            std::forward<Arg4>(a4), 
            std::forward<Arg5>(a5), 
            std::forward<Arg6>(a6)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3,
        typename Arg4, typename Arg5, typename Arg6, typename Arg7>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, Arg3&& a3, Arg4&& a4, Arg5&& a5, Arg6&& a6, Arg7&& a7)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1),
            std::forward<Arg2>(a2), 
            std::forward<Arg3>(a3), 
            std::forward<Arg4>(a4), 
            std::forward<Arg5>(a5), 
            std::forward<Arg6>(a6), 
            std::forward<Arg7>(a7)))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory and 
      // take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, 
        typename Arg3, typename Arg4, typename Arg5, 
        typename Arg6, typename Arg7, typename Arg8>
      handler_ptr(raw_ptr_type& raw_ptr, Arg1&& a1, Arg2&& a2, 
        Arg3&& a3,Arg4&& a4, Arg5&& a5, Arg6&& a6, Arg7&& a7, Arg8&& a8)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(
            std::forward<Arg1>(a1), 
            std::forward<Arg2>(a2), 
            std::forward<Arg3>(a3), 
            std::forward<Arg4>(a4), 
            std::forward<Arg5>(a5), 
            std::forward<Arg6>(a6), 
            std::forward<Arg7>(a7), 
            std::forward<Arg8>(a8)))
      {
        raw_ptr.pointer_ = 0;
      }
#else
      // Construct object in raw memory and 
      // take ownership if construction succeeds.
      template <typename Arg1>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2, const Arg3& a3)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3, a4))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2, 
        const Arg3& a3, const Arg4& a4, const Arg5& a5)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3, a4, a5))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2, 
        const Arg3& a3, const Arg4& a4, const Arg5& a5, const Arg6& a6)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3, a4, a5, a6))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory 
      // and take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, typename Arg3, 
        typename Arg4, typename Arg5, typename Arg6, typename Arg7>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1,
        const Arg2& a2, const Arg3& a3, const Arg4& a4, 
        const Arg5& a5, const Arg6& a6, const Arg7& a7)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3, a4, a5, a6, a7))
      {
        raw_ptr.pointer_ = 0;
      }

      // Construct object in raw memory and 
      // take ownership if construction succeeds.
      template <typename Arg1, typename Arg2, 
        typename Arg3, typename Arg4, typename Arg5, 
        typename Arg6, typename Arg7, typename Arg8>
      handler_ptr(raw_ptr_type& raw_ptr, const Arg1& a1, const Arg2& a2, 
        const Arg3& a3, const Arg4& a4, const Arg5& a5, 
        const Arg6& a6, const Arg7& a7, const Arg8& a8)
        : alloc_context_(boost::addressof(raw_ptr.alloc_context_))
        , pointer_(new (raw_ptr.pointer_) value_type(a1, a2, a3, a4, a5, a6, a7, a8))
      {
        raw_ptr.pointer_ = 0;
      }
#endif // defined(MA_HAS_RVALUE_REFS)

      // Destructor automatically deallocates memory, 
      // unless it has been released.
      ~handler_ptr()
      {
        reset();
      }

      // Get the memory.
      pointer_type get() const
      {
        return pointer_;
      }

      // Change allocation context used for memory deallocation
      // Never throws
      void set_alloc_context(alloc_context_type& alloc_context)
      {
        alloc_context_ = boost::addressof(alloc_context);
      }

      // Release ownership of the memory.
      pointer_type release()
      {
        pointer_type tmp = pointer_;
        pointer_ = 0;
        return tmp;
      }

      // Explicitly destroy and deallocate the memory.
      void reset()
      {
        if (pointer_)
        {
          // Move memory ownership to guard
          raw_ptr_type raw_ptr(*alloc_context_, pointer_);          
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

#endif // MA_HANDLER_ALLOC_HELPERS_HPP