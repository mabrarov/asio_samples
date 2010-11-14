//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_DERIVED_HPP
#define MA_TUTORIAL_ASYNC_DERIVED_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <string>
#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <ma/handler_allocation.hpp>
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
      Async_derived(boost::asio::io_service& io_service, const std::string& name);
      ~Async_derived();      

    protected:
      boost::optional<boost::system::error_code> do_something();

    private:
      typedef Async_derived this_type;

      void handle_timer(const boost::system::error_code& error);
      
      std::size_t counter_;
      boost::asio::deadline_timer timer_;
      std::string name_;
      boost::format start_message_fmt_;
      boost::format cycle_message_fmt_;
      boost::format error_end_message_fmt_;
      boost::format success_end_message_fmt_;
      ma::in_place_handler_allocator<128> timer_allocator_;
    }; // class Async_derived

  } // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_DERIVED_HPP
