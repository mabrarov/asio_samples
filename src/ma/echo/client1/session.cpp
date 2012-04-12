//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/client1/error.hpp>
#include <ma/echo/client1/session.hpp>

namespace ma {
namespace echo {
namespace client1 {

session::session(boost::asio::io_service& io_service,
    const session_options& options)
  : socket_recv_buffer_size_(options.socket_recv_buffer_size())
  , socket_send_buffer_size_(options.socket_send_buffer_size())
  , no_delay_(options.no_delay())
  , socket_write_in_progress_(false)
  , socket_read_in_progress_(false)
  , state_(external_state::ready)
  , io_service_(io_service)
  , strand_(io_service)
  , socket_(io_service)
  , wait_handler_(io_service)
  , stop_handler_(io_service)
  , buffer_(options.buffer_size())
{
}

void session::reset()
{
  boost::system::error_code ignored;
  socket_.close(ignored);
  wait_error_.clear();
  stop_error_.clear();
  state_ = external_state::ready;
  buffer_.reset();
}

} // namespace client1
} // namespace echo
} // namespace ma
