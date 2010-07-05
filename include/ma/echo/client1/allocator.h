//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_ALLOCATOR_H
#define MA_ECHO_CLIENT1_ALLOCATOR_H

#include <boost/utility.hpp>
#include <ma/echo/client1/allocator_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {
      class allocator : private boost::noncopyable
      {
      public:
        virtual void* allocate(std::size_t size) = 0;
        virtual void  deallocate(void* pointer)  = 0;

      protected:
        virtual ~allocator()
        {
        }
      }; // class allocator

    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_ALLOCATOR_H
