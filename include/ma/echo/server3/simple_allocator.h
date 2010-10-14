//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_SIMPLE_ALLOCATOR_H
#define MA_ECHO_SERVER3_SIMPLE_ALLOCATOR_H

#include <cstddef>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server3/allocator.h>

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      class simple_allocator : public allocator        
      {
      public:
        explicit simple_allocator(std::size_t size = in_heap_handler_allocator::default_size);

        virtual ~simple_allocator()
        {
        } // ~simple_allocator      

        void* allocate(std::size_t size);
        void deallocate(void* pointer);
        
      private:
        in_heap_handler_allocator alloc_;
      }; // class allocator

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_SIMPLE_ALLOCATOR_H
