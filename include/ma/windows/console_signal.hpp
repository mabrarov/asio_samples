//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WINDOWS_CONSOLE_SIGNAL_HPP
#define MA_WINDOWS_CONSOLE_SIGNAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/windows/console_signal_service.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>


#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace windows {

class console_signal : private boost::noncopyable
{
private:
  typedef console_signal this_type;

public:
  typedef console_signal_service            service_type;
  typedef service_type::implementation_type implementation_type;
  
  explicit console_signal(boost::asio::io_service& io_service);
  ~console_signal();

  boost::asio::io_service& get_io_service();

#if defined(MA_HAS_RVALUE_REFS)
  
  template <typename Handler>
  void async_wait(Handler&& handler);

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_wait(const Handler& handler);

#endif // defined(MA_HAS_RVALUE_REFS)

  std::size_t cancel(boost::system::error_code& error);
  
private:
  service_type&       service_;
  implementation_type impl_;
}; // class console_signal

console_signal::console_signal(boost::asio::io_service& io_service)
  : service_(boost::asio::use_service<service_type>(io_service))
{
  service_.construct(impl_);
}

console_signal::~console_signal()
{
  service_.destroy(impl_);
}

boost::asio::io_service& console_signal::get_io_service()
{
  return service_.get_io_service();
}

#if defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
void console_signal::async_wait(Handler&& handler)
{
  service_.async_wait(impl_, std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

template<typename Handler>
void console_signal::async_wait(const Handler& handler)
{
  service_.async_wait(impl_, handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

std::size_t console_signal::cancel(boost::system::error_code& error)
{
  return service_.cancel(impl_, error);
}

} // namespace windows
} // namespace ma

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#endif // MA_WINDOWS_CONSOLE_SIGNAL_HPP
