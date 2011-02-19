//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/scoped_array.hpp>

namespace ma
{  
  template <std::size_t alloc_size>
  class in_place_handler_allocator : private boost::noncopyable
  {  
  public:
    in_place_handler_allocator()
      : in_use_(false)
    {
    }

    ~in_place_handler_allocator()
    {
    }

    void* allocate(std::size_t size)
    {
      if (!in_use_ && size <= storage_.size)
      {
        in_use_ = true;
        return storage_.address();
      }      
      return ::operator new(size);      
    }

    void deallocate(void* pointer)
    {
      if (pointer == storage_.address())
      {        
        in_use_ = false;
        return;
      }      
      ::operator delete(pointer);      
    }

  private:    
    boost::aligned_storage<alloc_size> storage_;    
    bool in_use_;
  }; //class in_place_handler_allocator
  
  class in_heap_handler_allocator : private boost::noncopyable
  {  
  private:
    typedef char byte_type;

    static byte_type* allocate_storage(std::size_t size)
    {      
      std::size_t alloc_size = size;      
      return new byte_type[alloc_size];      
    }

    bool storage_initialized() const
    {
      return 0 != storage_.get();
    }

    byte_type* retrieve_aligned_address()
    {
      if (!storage_.get())
      {
        storage_.reset(allocate_storage(size_));
      }      
      return storage_.get();
    }

  public:    
    explicit in_heap_handler_allocator(std::size_t size, bool lazy = false)
      : storage_(lazy ? 0 : allocate_storage(size))      
      , size_(size)      
      , in_use_(false)
    {
    }

    ~in_heap_handler_allocator()
    {
    }

    void* allocate(std::size_t size)
    {
      if (!in_use_ && size <= size_)
      {        
        in_use_ = true;
        return retrieve_aligned_address();
      }      
      return ::operator new(size);      
    }

    void deallocate(void* pointer)
    {
      if (storage_initialized())
      {
        if (pointer == retrieve_aligned_address())
        {
          in_use_ = false;
          return;
        }
      }      
      ::operator delete(pointer);      
    }

  private:    
    boost::scoped_array<byte_type> storage_;    
    std::size_t size_;    
    bool in_use_;
  }; //class in_heap_handler_allocator  

} //namespace ma

#endif // MA_HANDLER_ALLOCATOR_HPP