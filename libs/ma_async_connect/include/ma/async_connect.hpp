//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
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
#include <ma/config.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/utility.hpp>

#if defined(WIN32) && !defined(BOOST_ASIO_DISABLE_IOCP) \
    && !defined(MA_BOOST_ASIO_WINDOWS_CONNECT_EX) && (_WIN32_WINNT >= 0x0501)
#define MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX
#else
#undef  MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX
#endif

#if defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)
#include <mswsock.h>
#include <winsock2.h>
#include <cstddef>
#include <boost/numeric/conversion/cast.hpp>
#include <ma/bind_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#endif // defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

namespace ma {

#if defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

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

  template <typename H>
  explicit connect_ex_handler(MA_FWD_REF(H) handler)
    : handler_(detail::forward<H>(handler))
  {
  }

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  connect_ex_handler(this_type&& other)
    : handler_(detail::move(other.handler_))
  {
  }

  connect_ex_handler(const this_type& other)
    : handler_(other.handler_)
  {
  }

#endif

#if !defined(NDEBUG)
  ~connect_ex_handler()
  {
  }
#endif

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    return ma_handler_alloc_helpers::allocate(size, context->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    ma_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function, 
      this_type* context)
  {
    ma_handler_invoke_helpers::invoke(
        detail::forward<Function>(function), context->handler_);
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    ma_handler_invoke_helpers::invoke(function, context->handler_);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  friend bool asio_handler_is_continuation(this_type* context)
  {
    return ma_handler_cont_helpers::is_continuation(context->handler_);
  }

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

template <typename Handler>
inline connect_ex_handler<typename detail::decay<Handler>::type>
make_connect_ex_handler(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  return connect_ex_handler<handler_type>(detail::forward<Handler>(handler));
}

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

#endif // defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

template <typename Socket, typename Handler>
void async_connect(Socket& socket,
    const typename Socket::endpoint_type& peer_endpoint, 
    MA_FWD_REF(Handler) handler)
{
#if defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

  if (!socket.is_open())
  {
    // Open the socket before use ConnectEx.
    boost::system::error_code error;
    socket.open(peer_endpoint.protocol(), error);
    if (error)
    {
      socket.get_io_service().post(
          ma::bind_handler(detail::forward<Handler>(handler), error));
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
    socket.async_connect(peer_endpoint, detail::forward<Handler>(handler));
    return;
  }

  if (boost::system::error_code error = detail::bind_to_any(socket, 
      peer_endpoint.protocol()))
  {
    socket.get_io_service().post(
        ma::bind_handler(detail::forward<Handler>(handler), error));
    return;
  }

  // Construct an OVERLAPPED-derived object to contain the handler.
  boost::asio::windows::overlapped_ptr overlapped(socket.get_io_service(),
      detail::make_connect_ex_handler(detail::forward<Handler>(handler)));

  // Initiate the ConnectEx operation.
  BOOL ok = connect_ex_func(native_socket, peer_endpoint.data(),
      boost::numeric_cast<int>(peer_endpoint.size()), NULL, 0, NULL,
      overlapped.get());
  int last_error = ::WSAGetLastError();

  // Check if the operation completed immediately.
  if ((FALSE == ok) && (ERROR_IO_PENDING != last_error))
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

#else // defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

  socket.async_connect(peer_endpoint, detail::forward<Handler>(handler));

#endif // defined(MA_ASYNC_CONNECT_USES_WINDOWS_CONNECT_EX)

}

} // namespace ma

#endif // MA_ASYNC_CONNECT_HPP
