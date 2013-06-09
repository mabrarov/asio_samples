//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
#define MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <ma/config.hpp>
#include <ma/type_traits.hpp>
#include <ma/bind_handler.hpp>
#include <ma/detail/handler_ptr.hpp>
#include <ma/detail/service_base.hpp>
#include <ma/detail/intrusive_list.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace windows {

/// asio::io_service::service implementing console_signal.
class console_signal_service
  : public detail::service_base<console_signal_service>
{
private:
  // Base class to hold up any value.
  class stored_base;

  // Base class for implementation that helps to hide
  // public inheritance from detail::intrusive_list::base_hook
  class impl_base
    : public detail::intrusive_list<impl_base>::base_hook
    , private boost::noncopyable
  {
  public:
    impl_base();

#if !defined(NDEBUG)
    ~impl_base();
#endif

  private:
    friend class console_signal_service;
  }; // class impl_base

public:
  class implementation_type : private impl_base
  {
  private:
    friend class console_signal_service;
  }; // class implementation_type

  explicit console_signal_service(boost::asio::io_service& io_service);
  void construct(implementation_type& impl);
  void destroy(implementation_type& impl);

  template <typename Handler>
  void async_wait(implementation_type& impl, Handler handler);

  std::size_t cancel(const implementation_type& impl, 
      boost::system::error_code& error);

protected:
  virtual ~console_signal_service();

private:
  virtual void shutdown_service();
}; // class handler_storage_service

inline console_signal_service::console_signal_service(
    boost::asio::io_service& io_service)
  : detail::service_base<console_signal_service>(io_service)  
{
  //todo
}

inline void console_signal_service::construct(implementation_type& impl)
{
  //todo
}

inline void console_signal_service::destroy(implementation_type& impl)
{
  //todo
}

template <typename Handler>
inline void console_signal_service::async_wait(
    implementation_type& impl, Handler handler)
{
  //todo
}

inline std::size_t console_signal_service::cancel(
    const implementation_type& impl, boost::system::error_code& error)
{
  //todo
}

} // namespace windows
} // namespace ma

#endif // MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
