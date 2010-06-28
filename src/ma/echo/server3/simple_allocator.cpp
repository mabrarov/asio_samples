//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/server3/simple_allocator.h>

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      simple_allocator::simple_allocator(std::size_t size)
        : alloc_(size)
      {
      } // simple_allocator::simple_allocator

      void* simple_allocator::allocate(std::size_t size)
      {
        return alloc_.allocate(size);
      } // simple_allocator::allocate

      void simple_allocator::deallocate(void* pointer)
      {
        alloc_.deallocate(pointer);
      } // simple_allocator::deallocate

      simple_allocator::~simple_allocator()
      {
      } // simple_allocator::~simple_allocator      

    } // namespace server2
  } // namespace echo
} // namespace ma