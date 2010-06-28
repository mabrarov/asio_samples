//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_ALLOCATOR_H
#define MA_ECHO_SERVER3_ALLOCATOR_H

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      class allocator
      {
      public:
        virtual void* allocate(std::size_t size) = 0;
        virtual void  deallocate(void* pointer)  = 0;

      protected:
        virtual ~allocator()
        {
        }
      }; // class allocator

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_ALLOCATOR_H
