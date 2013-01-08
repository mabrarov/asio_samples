//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL2_DO_SOMETHING_HANDLER_HPP
#define MA_TUTORIAL2_DO_SOMETHING_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdlib>
#include <boost/system/error_code.hpp>
#include <ma/tutorial2/do_something_handler_fwd.hpp>

namespace ma {
namespace tutorial2 {

class do_something_handler
{
private:
  typedef do_something_handler this_type;

public:
  virtual void handle_do_something_completion(
      const boost::system::error_code&) = 0;

  // Default implementations for member functions that
  // are often implemented in the same way.
  virtual void* allocate(std::size_t);
  virtual void deallocate(void*);

protected:

  // No-op member functions
  do_something_handler();
  do_something_handler(const this_type&);
  this_type& operator=(const this_type&);
  ~do_something_handler();

}; // class do_something_handler

inline void* do_something_handler::allocate(std::size_t size)
{
  return ::operator new(size);
}

inline void do_something_handler::deallocate(void* pointer)
{
  ::operator delete(pointer);
}

inline do_something_handler::do_something_handler()
{
}

inline do_something_handler::do_something_handler(const this_type&)
{
}

inline do_something_handler::this_type& do_something_handler::operator=(
    const this_type&)
{
  return *this;
}

inline do_something_handler::~do_something_handler()
{
}

} // namespace tutorial2
} // namespace ma

#endif // MA_TUTORIAL2_DO_SOMETHING_HANDLER_HPP
