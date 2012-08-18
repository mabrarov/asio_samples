//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ASYNC_CONNECT_HPP
#define MA_ASYNC_CONNECT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>

#if defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)
#include <mswsock.h>
#include <winsock2.h>
#include <cstddef>
#include <boost/numeric/conversion/cast.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#endif // defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)

#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

#if defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)

namespace detail {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler>
class connect_ex_handler
{
private:
  typedef connect_ex_handler<Handler> this_type;

public:
  typedef void result_type;

#if defined(MA_HAS_RVALUE_REFS)

  template <typename H>
  explicit connect_ex_handler(H&& handler)
    : handler_(std::forward<H>(handler))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  connect_ex_handler(this_type&& other)
    : handler_(std::move(other.handler_))
  {
  }

  connect_ex_handler(const this_type& other)
    : handler_(other.handler_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  explicit connect_ex_handler(const Handler& handler)
    : handler_(handler)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~connect_ex_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_asio_handler_alloc_helpers::deallocate(pointer, size,
        context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function&& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(std::forward<Function>(function),
        context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  void operator()(const boost::system::error_code& error,
      std::size_t /*bytes_transferred*/)
  {
    handler_(error);
  }

  void operator()(const boost::system::error_code& error,
      std::size_t /*bytes_transferred*/) const
  {
    handler_(error);
  }

private:
  Handler handler_;
}; // class connect_ex_handler

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
inline connect_ex_handler<typename remove_cv_reference<Handler>::type>
make_connect_ex_handler(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  return connect_ex_handler<handler_type>(std::forward<Handler>(handler));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
inline connect_ex_handler<Handler>
make_connect_ex_handler(const Handler& handler)
{
  return connect_ex_handler<Handler>(handler);
}

#endif // defined(MA_HAS_RVALUE_REFS)

template <typename Socket>
boost::system::error_code bind_to_any(Socket& socket, 
    const typename Socket::endpoint_type::protocol_type& protocol)
{
  typedef typename Socket::endpoint_type endpoint_type;

  static const boost::system::error_code ignored(WSAEINVAL,
      boost::asio::error::get_system_category());

  boost::system::error_code error;
  socket.bind(endpoint_type(protocol, 0), error);

  if (ignored == error)
  {
    error.clear();
  }

  return error;
}

} // namespace detail

#endif // defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)

#if defined(MA_HAS_RVALUE_REFS)

template <typename Socket, typename Handler>
void async_connect(Socket& socket,
    const typename Socket::endpoint_type& peer_endpoint, Handler&& handler)

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Socket, typename Handler>
void async_connect(Socket& socket,
    const typename Socket::endpoint_type& peer_endpoint,
    const Handler& handler)

#endif // defined(MA_HAS_RVALUE_REFS)
{
#if defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)

#if (_WIN32_WINNT < 0x0501)
#error The build environment does not support necessary Windows SDK header.\
    Value of _WIN32_WINNT macro must be >= 0x0501.
#endif

  if (!socket.is_open())
  {
    // Open the socket before use ConnectEx.
    boost::system::error_code error;
    socket.open(peer_endpoint.protocol(), error);
    if (error)
    {
#if defined(MA_HAS_RVALUE_REFS)

      socket.get_io_service().post(
          detail::bind_handler(std::forward<Handler>(handler), error));

#else // defined(MA_HAS_RVALUE_REFS)

      socket.get_io_service().post(detail::bind_handler(handler, error));

#endif // defined(MA_HAS_RVALUE_REFS)
      return;
    }
  }

#if BOOST_ASIO_VERSION < 100600
  SOCKET native_socket = socket.native();
#else
  SOCKET native_socket = socket.native_handle();
#endif
  GUID connect_ex_guid = WSAID_CONNECTEX;
  LPFN_CONNECTEX connect_ex_func = 0;
  DWORD result_bytes;

  int ctrl_result = ::WSAIoctl(native_socket,
      SIO_GET_EXTENSION_FUNCTION_POINTER,
      &connect_ex_guid, sizeof(connect_ex_guid),
      &connect_ex_func, sizeof(connect_ex_func),
      &result_bytes, NULL, NULL);

  // If ConnectEx wasn't located then fall to common Asio async_connect.
  if ((SOCKET_ERROR == ctrl_result) || !connect_ex_func)
  {
#if defined(MA_HAS_RVALUE_REFS)

    socket.async_connect(peer_endpoint, std::forward<Handler>(handler));

#else // defined(MA_HAS_RVALUE_REFS)

    socket.async_connect(peer_endpoint, handler);

#endif // defined(MA_HAS_RVALUE_REFS)
    return;
  }

  if (boost::system::error_code error = detail::bind_to_any(socket, 
      peer_endpoint.protocol()))
  {
#if defined(MA_HAS_RVALUE_REFS)

    socket.get_io_service().post(
        detail::bind_handler(std::forward<Handler>(handler), error));

#else // defined(MA_HAS_RVALUE_REFS)

    socket.get_io_service().post(detail::bind_handler(handler, error));

#endif // defined(MA_HAS_RVALUE_REFS)
    return;
  }

#if defined(MA_HAS_RVALUE_REFS)

  // Construct an OVERLAPPED-derived object to contain the handler.
  boost::asio::windows::overlapped_ptr overlapped(socket.get_io_service(),
      detail::make_connect_ex_handler(std::forward<Handler>(handler)));

#else // defined(MA_HAS_RVALUE_REFS)

  // Construct an OVERLAPPED-derived object to contain the handler.
  boost::asio::windows::overlapped_ptr overlapped(socket.get_io_service(),
      detail::make_connect_ex_handler(handler));

#endif // defined(MA_HAS_RVALUE_REFS)

  // Initiate the ConnectEx operation.
  BOOL ok = connect_ex_func(native_socket, peer_endpoint.data(),
      boost::numeric_cast<int>(peer_endpoint.size()), NULL, 0, NULL,
      overlapped.get());
  DWORD last_error = ::WSAGetLastError();

  // Check if the operation completed immediately.
  if ((TRUE != ok) && (ERROR_IO_PENDING != last_error))
  {
    // The operation completed immediately, so a completion notification needs
    // to be posted. When complete() is called, ownership of the OVERLAPPED-
    // derived object passes to the io_service.
    boost::system::error_code error(last_error,
        boost::asio::error::get_system_category());
    overlapped.complete(error, 0);
  }
  else
  {
    // The operation was successfully initiated, so ownership of the
    // OVERLAPPED-derived object has passed to the io_service.
    overlapped.release();
  }

#else // defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP)

#if defined(MA_HAS_RVALUE_REFS)

  socket.async_connect(peer_endpoint, std::forward<Handler>(handler));

#else // defined(MA_HAS_RVALUE_REFS)

  socket.async_connect(peer_endpoint, handler);

#endif // defined(MA_HAS_RVALUE_REFS)

#endif // defined(WIN32)

}

} // namespace ma

#endif // MA_ASYNC_CONNECT_HPP
