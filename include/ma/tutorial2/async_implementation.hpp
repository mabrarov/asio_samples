//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL2_ASYNC_IMPLEMENTATION_HPP
#define MA_TUTORIAL2_ASYNC_IMPLEMENTATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/tutorial2/async_interface.hpp>

namespace ma {
namespace tutorial2 {

class async_implementation
  : private boost::noncopyable
  , public async_interface
  , public boost::enable_shared_from_this<async_implementation>
{
private:
  typedef async_implementation this_type;

public:
  static async_interface_ptr create(boost::asio::io_service& io_service, 
      const std::string& name);
  virtual void async_do_something(const do_something_handler_ptr&);

protected:
  async_implementation(boost::asio::io_service& io_service,
      const std::string& name);
  ~async_implementation();

private:
  void complete_do_something(const boost::system::error_code&);
  bool has_do_something_handler() const;
  void start_do_something(const do_something_handler_ptr&);
  boost::optional<boost::system::error_code> do_start_do_something();
  void handle_timer(const boost::system::error_code&);

  boost::asio::io_service::strand strand_;
  ma::handler_storage<boost::system::error_code> do_something_handler_;

  std::size_t counter_;
  steady_deadline_timer timer_;

  std::string name_;
  boost::format start_message_fmt_;
  boost::format cycle_message_fmt_;
  boost::format error_end_message_fmt_;
  boost::format success_end_message_fmt_;

  ma::in_place_handler_allocator<128> timer_allocator_;
}; // class async_implementation

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL2_ASYNC_IMPLEMENTATION_HPP
