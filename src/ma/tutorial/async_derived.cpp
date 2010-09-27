//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/tutorial/async_derived.hpp>

namespace ma
{
  namespace tutorial
  {    
    Async_derived::Async_derived(boost::asio::io_service& io_service)
      : Strand_base(boost::ref(io_service))
      , Async_base(Strand_base::member)
    {
    } // Async_derived::Async_derived

    Async_derived::~Async_derived()
    {
    } // Async_derived::~Async_derived
    
    boost::optional<boost::system::error_code> Async_derived::do_something()
    {
      //bla..bla..bla
      return boost::system::error_code();    
    } // Async_derived::do_something

  } // namespace tutorial
} // namespace ma
