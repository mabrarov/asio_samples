//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONSOLE_CLOSE_SIGNAL_HPP
#define MA_CONSOLE_CLOSE_SIGNAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/console_close_signal_service.hpp>

#if !defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
#include <csignal>
#endif

#include <boost/asio.hpp>

#if !defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>
#endif

#include <boost/noncopyable.hpp>
#include <ma/config.hpp>

#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
#include <ma/io_context_helpers.hpp>
#endif

#include <ma/detail/utility.hpp>

namespace ma {

class console_close_signal : private boost::noncopyable
{
private:
  typedef console_close_signal this_type;

public:
#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  typedef console_close_signal_service      service_type;
  typedef service_type::implementation_type implementation_type;
#endif

  explicit console_close_signal(boost::asio::io_service& io_service);

#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  ~console_close_signal();
#endif

  boost::asio::io_service& get_io_service();

  template <typename Handler>
  void async_wait(MA_FWD_REF(Handler) handler);

  void cancel(boost::system::error_code& error);

private:
#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  service_type&       service_;
  implementation_type impl_;
#else
  static void throw_if_error(const boost::system::error_code& error);
#if BOOST_VERSION >= 107000
  boost::asio::io_service& io_service_;
#endif
  boost::asio::signal_set signal_set_;
#endif
}; // class console_close_signal

#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)

inline console_close_signal::console_close_signal(
    boost::asio::io_service& io_service)
  : service_(boost::asio::use_service<service_type>(io_service))
{
  service_.construct(impl_);
}

#else // defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)

inline console_close_signal::console_close_signal(
    boost::asio::io_service& io_service)
#if BOOST_VERSION >= 107000
  : io_service_(io_service)
  , signal_set_(io_service)
#else
  : signal_set_(io_service)
#endif
{
  boost::system::error_code error;
  signal_set_.add(SIGINT, error);
  throw_if_error(error);

  signal_set_.add(SIGTERM, error);
  throw_if_error(error);

#if defined(SIGQUIT)
  signal_set_.add(SIGQUIT, error);
  throw_if_error(error);
#endif
}

#endif // defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)

#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)

inline console_close_signal::~console_close_signal()
{
  service_.destroy(impl_);
}

#endif // defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)

inline boost::asio::io_service& console_close_signal::get_io_service()
{
#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  return ma::get_io_context(service_);
#elif BOOST_VERSION >= 107000
  return io_service_;
#else
  return signal_set_.get_io_service();
#endif
}

template <typename Handler>
void console_close_signal::async_wait(MA_FWD_REF(Handler) handler)
{
#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  service_.async_wait(impl_, detail::forward<Handler>(handler));
#else
  signal_set_.async_wait(detail::forward<Handler>(handler));
#endif
}

inline void console_close_signal::cancel(boost::system::error_code& error)
{
#if defined(MA_HAS_CONSOLE_CLOSE_SIGNAL_SERVICE)
  service_.cancel(impl_, error);
#else
  signal_set_.cancel(error);
#endif
}

#if !defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

inline void console_close_signal::throw_if_error(
    const boost::system::error_code& error)
{
  if (error)
  {
    boost::throw_exception(boost::system::system_error(error));
  }
}

#endif // !defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

} // namespace ma

#endif // MA_CONSOLE_CLOSE_SIGNAL_HPP
