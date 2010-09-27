//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_DERIVED_HPP
#define MA_TUTORIAL_ASYNC_DERIVED_HPP

#include <boost/utility.hpp>
#include <ma/tutorial/async_base.hpp>

namespace ma
{
  namespace tutorial
  {    
    class Async_derived 
      : private boost::base_from_member<boost::asio::io_service::strand>
      , public Async_base
    {
    private:
      typedef boost::base_from_member<boost::asio::io_service::strand> Strand_base;

    public:
      Async_derived(boost::asio::io_service& io_service);
      ~Async_derived();      

    protected:
      boost::optional<boost::system::error_code> do_something();
    }; // class Async_derived

  } // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_DERIVED_HPP
