//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONSOLE_CLOSE_SIGNAL_HPP
#define MA_CONSOLE_CLOSE_SIGNAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/detail/utility.hpp>
#include <ma/console_close_signal_service.hpp>

namespace ma {

class console_close_signal : private boost::noncopyable
{
private:
  typedef console_close_signal this_type;

public:
  typedef console_close_signal_service      service_type;
  typedef service_type::implementation_type implementation_type;

  explicit console_close_signal(boost::asio::io_service& io_service);
  ~console_close_signal();

  boost::asio::io_service& get_io_service();

  template <typename Handler>
  void async_wait(MA_FWD_REF(Handler) handler);

  void cancel(boost::system::error_code& error);

private:
  service_type&       service_;
  implementation_type impl_;
}; // class console_close_signal

inline console_close_signal::console_close_signal(
    boost::asio::io_service& io_service)
  : service_(boost::asio::use_service<service_type>(io_service))
{
  service_.construct(impl_);
}

inline console_close_signal::~console_close_signal()
{
  service_.destroy(impl_);
}

inline boost::asio::io_service& console_close_signal::get_io_service()
{
  return service_.get_io_service();
}

template <typename Handler>
void console_close_signal::async_wait(MA_FWD_REF(Handler) handler)
{
  service_.async_wait(impl_, detail::forward<Handler>(handler));
}

inline void console_close_signal::cancel(boost::system::error_code& error)
{
  service_.cancel(impl_, error);
}

} // namespace ma

#endif // MA_CONSOLE_CLOSE_SIGNAL_HPP
