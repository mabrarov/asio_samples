//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONSOLE_CLOSE_GUARD_HPP
#define MA_CONSOLE_CLOSE_GUARD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <ma/config.hpp>
#include <ma/type_traits.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/console_close_signal.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

class console_close_guard_base : private boost::noncopyable
{
public:
  template <typename Handler>
  explicit console_close_guard_base(MA_FWD_REF(Handler) handler);

protected:
  ~console_close_guard_base();

  template <typename Handler>
  static void start_wait(console_close_signal& close_signal,
      MA_FWD_REF(Handler) handler);

  template <typename Handler>
  static void handle_signal(console_close_signal& close_signal,
      Handler& handler, const boost::system::error_code& error);

  boost::asio::io_service io_service_;
  console_close_signal    close_signal_;
}; // class console_close_guard_base

/// Hook-helper for console application. Supports setup of the own functor that
/// will be called when user tries to close console application.
/**
 * Supported OS: MS Windows family, Linux (Ubuntu 10.x tested).
 */
class console_close_guard : private console_close_guard_base
{
public:
  template <typename Handler>
  explicit console_close_guard(MA_FWD_REF(Handler) handler);
  ~console_close_guard();

private:
  detail::thread work_thread_;
}; // class console_close_guard

template <typename Handler>
console_close_guard_base::console_close_guard_base(MA_FWD_REF(Handler) handler)
  : io_service_(1)
  , close_signal_(io_service_)
{
  start_wait(close_signal_, detail::forward<Handler>(handler));
}

inline console_close_guard_base::~console_close_guard_base()
{
}

template <typename Handler>
void console_close_guard_base::start_wait(console_close_signal& close_signal,
    MA_FWD_REF(Handler) handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  close_signal.async_wait(make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(&handle_signal<handler_type>, detail::ref(close_signal),
          detail::placeholders::_1, detail::placeholders::_2)));
}

template <typename Handler>
void console_close_guard_base::handle_signal(console_close_signal& close_signal,
    Handler& handler, const boost::system::error_code& error)
{
  if (boost::asio::error::operation_aborted != error)
  {
    start_wait(close_signal, handler);
    // This can cause handler associated allocator to be used
    // for the second time while above call makes it being used
    // for the first time, i.e. asio_handler_allocate can be invoked
    // twice without asio_handler_deallocate between (but the number
    // of calls of asio_handler_allocate will keep being the same
    // as the number of calls of asio_handler_deallocate).
    close_signal.get_io_service().dispatch(handler);
  }
}

template <typename Handler>
console_close_guard::console_close_guard(MA_FWD_REF(Handler) handler)
  : console_close_guard_base(detail::forward<Handler>(handler))
  , work_thread_(detail::bind(
        static_cast<std::size_t (boost::asio::io_service::*)(void)>(
            &boost::asio::io_service::run), detail::ref(io_service_)))
{
}

inline console_close_guard::~console_close_guard()
{
  io_service_.stop();
  work_thread_.join();
}

} // namespace ma

#endif // MA_CONSOLE_CLOSE_GUARD_HPP
