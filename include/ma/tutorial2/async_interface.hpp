//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL2_ASYNC_INTERFACE_HPP
#define MA_TUTORIAL2_ASYNC_INTERFACE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/tutorial2/do_something_handler_fwd.hpp>

namespace ma {

namespace tutorial2 {

class async_interface
{
private:
  typedef async_interface this_type;

public:
  virtual void async_do_something(const do_something_handler_ptr&) = 0;

protected:
  async_interface()
  {
  }

  async_interface(const this_type&)
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }

  ~async_interface()
  {
  }

}; // class async_interface

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL2_ASYNC_INTERFACE_HPP
