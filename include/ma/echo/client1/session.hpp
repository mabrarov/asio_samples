//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_HPP
#define MA_ECHO_CLIENT1_SESSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/echo/client1/session_options.hpp>
#include <ma/echo/client1/session_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace echo {

namespace client1 {

class session
  : private boost::noncopyable
  , public boost::enable_shared_from_this<session>
{
private:
  typedef session this_type;

public:
  session(boost::asio::io_service& io_service,
    const session_options& options);

  ~session()
  {
  }

  boost::asio::ip::tcp::socket& socket()
  {
    return socket_;
  }

  void reset();

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(&this_type::start_external_start<handler_type>,
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(&this_type::start_external_stop<handler_type>,
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(&this_type::start_external_wait<handler_type>,
            shared_from_this(), _1)));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, boost::bind(
        &this_type::start_external_start<Handler>, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, boost::bind(
        &this_type::start_external_stop<Handler>, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, boost::bind(
        &this_type::start_external_wait<Handler>, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:
  struct external_state
  {
    enum value_t
    {
      ready,
      start,
      started,
      stop,
      stopped
    };
  }; // struct external_state

  template <typename Handler>
  void start_external_start(const Handler& handler)
  {
    //todo
    io_service_.post(detail::bind_handler(handler,
        boost::asio::error::operation_not_supported));
    //boost::system::error_code error = do_start_external_start();
    //io_service_.post(detail::bind_handler(handler, error));
  }

  template <typename Handler>
  void start_external_stop(const Handler& handler)
  {
    //todo
    io_service_.post(detail::bind_handler(handler,
        boost::asio::error::operation_not_supported));
    //if (boost::optional<boost::system::error_code> result =
    //    do_start_external_stop())
    //{
    //  io_service_.post(detail::bind_handler(handler, *result));
    //}
    //else
    //{
    //  stop_handler_.store(handler);
    //}
  }

  template <typename Handler>
  void start_external_wait(const Handler& handler)
  {
    //todo
    io_service_.post(detail::bind_handler(handler,
        boost::asio::error::operation_not_supported));
    //if (boost::optional<boost::system::error_code> result =
    //    do_start_external_wait())
    //{
    //  io_service_.post(detail::bind_handler(handler, *result));
    //}
    //else
    //{
    //  wait_handler_.store(handler);
    //}
  }

  //todo
  //boost::system::error_code                  do_start_external_start();
  //boost::optional<boost::system::error_code> do_start_external_stop();
  //boost::optional<boost::system::error_code> do_start_external_wait();

  session_options::optional_int  socket_recv_buffer_size_;
  session_options::optional_int  socket_send_buffer_size_;
  session_options::optional_bool no_delay_;

  bool socket_write_in_progress_;
  bool socket_read_in_progress_;
  external_state::value_t state_;

  boost::asio::io_service& io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::ip::tcp::socket socket_;

  handler_storage<boost::system::error_code> wait_handler_;
  handler_storage<boost::system::error_code> stop_handler_;

  boost::system::error_code wait_error_;
  boost::system::error_code stop_error_;
  cyclic_buffer buffer_;

  in_place_handler_allocator<640> write_allocator_;
  in_place_handler_allocator<256> read_allocator_;
}; // class session

} // namespace client1
} // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_HPP
