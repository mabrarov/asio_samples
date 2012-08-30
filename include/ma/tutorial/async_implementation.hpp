//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_IMPLEMENTATION_HPP
#define MA_TUTORIAL_ASYNC_IMPLEMENTATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <string>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/tutorial/async_interface.hpp>

namespace ma {
namespace tutorial {

class async_implementation;
typedef boost::shared_ptr<async_implementation> async_implementation_ptr;

class async_implementation
  : private boost::noncopyable
  , public async_interface
  , public boost::enable_shared_from_this<async_implementation>
{
private:
  typedef async_implementation this_type;

public:
  static async_implementation_ptr create(boost::asio::io_service& io_service,
      const std::string& name);

protected:
  async_implementation(boost::asio::io_service& io_service,
      const std::string& name);

  ~async_implementation()
  {
  }

  boost::asio::io_service::strand& strand();
  do_something_handler_storage_type& do_something_handler_storage();
  boost::optional<boost::system::error_code> start_do_something();

private:
  void complete_do_something(const boost::system::error_code& error);
  bool has_do_something_handler() const;
  void handle_timer(const boost::system::error_code& error);

  boost::asio::io_service::strand   strand_;
  do_something_handler_storage_type do_something_handler_;
  steady_deadline_timer             timer_;
  std::size_t counter_;

  const std::string name_;
  boost::format start_message_fmt_;
  boost::format cycle_message_fmt_;
  boost::format error_end_message_fmt_;
  boost::format success_end_message_fmt_;

  ma::in_place_handler_allocator<128> timer_allocator_;
}; // class async_implementation

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_IMPLEMENTATION_HPP
