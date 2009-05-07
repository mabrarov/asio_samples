//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_ALLOC_HELPERS_HPP
#define MA_HANDLER_ALLOC_HELPERS_HPP

#include <cstddef>
#include <boost/asio.hpp>

namespace ma_alloc_helpers
{
  template <typename Context>
  inline void* allocate(std::size_t size, Context* context)
  {
    using namespace boost::asio;
    return asio_handler_allocate(size, context);
  }

  template <typename Context>
  inline void deallocate(void* pointer, std::size_t size, Context* context)
  {
    using namespace boost::asio;
    asio_handler_deallocate(pointer, size, context);
  }
}

#endif // MA_HANDLER_ALLOC_HELPERS_HPP