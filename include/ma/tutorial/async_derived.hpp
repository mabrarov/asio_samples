//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/tutorial/async_base.hpp>

namespace ma {
namespace tutorial {

class async_derived;
typedef boost::shared_ptr<async_derived> async_derived_ptr;

class async_derived
  : private boost::base_from_member<boost::asio::io_service::strand>
  , public async_base
  , public boost::enable_shared_from_this<async_derived>
{
private:
  typedef boost::base_from_member<boost::asio::io_service::strand> strand_base;
  typedef async_derived this_type;

public:
  static async_derived_ptr create(boost::asio::io_service& io_service,
      const std::string& name);

protected:
  async_derived(boost::asio::io_service& io_service, const std::string& name);
  ~async_derived();
  virtual async_base_ptr get_shared_base();
  virtual boost::optional<boost::system::error_code> do_start_do_something();

private:
  void handle_timer(const boost::system::error_code& error);

  std::size_t counter_;
  steady_deadline_timer timer_;

  const std::string name_;
  boost::format start_message_fmt_;
  boost::format cycle_message_fmt_;
  boost::format error_end_message_fmt_;
  boost::format success_end_message_fmt_;

  ma::in_place_handler_allocator<128> timer_allocator_;
}; // class async_derived

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_DERIVED_HPP
