//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/nmea/cyclic_read_session.hpp>

namespace ma {
namespace nmea {

cyclic_read_session_ptr cyclic_read_session::create(
    boost::asio::io_service& io_service, std::size_t read_buffer_size,
    std::size_t frame_buffer_size, const std::string& frame_head,
    const std::string& frame_tail)
{
  typedef shared_ptr_factory_helper<this_type> helper;
  return boost::make_shared<helper>(boost::ref(io_service), read_buffer_size,
      frame_buffer_size, frame_head, frame_tail);
}

cyclic_read_session::cyclic_read_session(boost::asio::io_service& io_service,
    std::size_t read_buffer_size, std::size_t frame_buffer_size,
    const std::string& frame_head, const std::string& frame_tail)
  : io_service_(io_service)
  , strand_(io_service)
  , serial_port_(io_service)
  , extern_read_handler_(io_service)
  , extern_stop_handler_(io_service)
  , frame_buffer_(frame_buffer_size)
  , port_write_in_progress_(false)
  , port_read_in_progress_(false)
  , extern_state_(extern_state::ready)
  , read_buffer_(read_buffer_size)
  , frame_head_(frame_head)
  , frame_tail_(frame_tail)
{
  if (frame_buffer_size < min_message_queue_size)
  {
    boost::throw_exception(
        std::invalid_argument("too small frame_buffer_size"));
  }

  if (read_buffer_size < max_message_size)
  {
    boost::throw_exception(
        std::invalid_argument("too small read_buffer_size"));
  }

  if (frame_head.length() > read_buffer_size)
  {
    boost::throw_exception(
        std::invalid_argument("too large frame_head"));
  }

  if (frame_tail.length() > read_buffer_size)
  {
    boost::throw_exception(
        std::invalid_argument("too large frame_tail"));
  }
}

void cyclic_read_session::resest()
{
  boost::system::error_code ignored;
  serial_port_.close(ignored);

  frame_buffer_.clear();
  read_error_.clear();

  read_buffer_.consume(boost::asio::buffer_size(read_buffer_.data()));

  extern_state_ = extern_state::ready;
}

boost::system::error_code cyclic_read_session::do_start_extern_start()
{
  if (extern_state::ready != extern_state_)
  {
    return nmea::error::invalid_state;
  }

  extern_state_ = extern_state::work;

  // Start internal activity
  if (!port_read_in_progress_)
  {
    read_until_head();
  }

  // Signal successful handshake completion.
  return boost::system::error_code();
}

cyclic_read_session::optional_error_code
cyclic_read_session::do_start_extern_stop()
{
  if ((extern_state::stopped == extern_state_)
      || (extern_state::stop == extern_state_))
  {
    return boost::system::error_code(nmea::error::invalid_state);
  }

  // Start shutdown
  extern_state_ = extern_state::stop;

  // Do shutdown - abort outer operations
  if (extern_read_handler_.has_target())
  {
    extern_read_handler_.post(
        read_result_type(nmea::error::operation_aborted, 0));
  }

  // Do shutdown - abort inner operations
  serial_port_.close(stop_error_);

  // Check for shutdown completion
  if (may_complete_stop())
  {
    complete_stop();
    // Signal shutdown completion
    return stop_error_;
  }

  return optional_error_code();
}

cyclic_read_session::optional_error_code
cyclic_read_session::do_start_extern_read_some()
{
  if ((extern_state::work != extern_state_)
      || (extern_read_handler_.has_target()))
  {
    return boost::system::error_code(nmea::error::invalid_state);
  }

  if (!frame_buffer_.empty())
  {
    // Signal that we can safely fill input buffer from the frame_buffer_
    return boost::system::error_code();
  }

  if (read_error_)
  {
    boost::system::error_code error = read_error_;
    read_error_.clear();
    return error;
  }

  // If can't immediately complete then do_start_extern_start waiting for completion
  // Start message constructing
  if (!port_read_in_progress_)
  {
    read_until_head();
  }
  return optional_error_code();
}

bool cyclic_read_session::may_complete_stop() const
{
  return !port_write_in_progress_ && !port_read_in_progress_;
}

void cyclic_read_session::complete_stop()
{
  extern_state_ = extern_state::stopped;
}

void cyclic_read_session::read_until_head()
{
  boost::asio::async_read_until(serial_port_, read_buffer_, frame_head_,
      MA_STRAND_WRAP(strand_, make_custom_alloc_handler(read_allocator_,
          boost::bind(&this_type::handle_read_head, shared_from_this(),
              _1, _2))));

  port_read_in_progress_ = true;
}

void cyclic_read_session::read_until_tail()
{
  boost::asio::async_read_until(serial_port_, read_buffer_, frame_tail_,
      MA_STRAND_WRAP(strand_, make_custom_alloc_handler(read_allocator_,
          boost::bind(&this_type::handle_read_tail, shared_from_this(),
              _1, _2))));

  port_read_in_progress_ = true;
}

void cyclic_read_session::handle_read_head(
    const boost::system::error_code& error,
    const std::size_t bytes_transferred)
{
  port_read_in_progress_ = false;

  // Check for pending session do_start_extern_stop operation
  if (extern_state::stop == extern_state_)
  {
    if (may_complete_stop())
    {
      complete_stop();
      post_extern_stop_handler();
    }
    return;
  }

  if (error)
  {
    // Check for pending session read operation
    if (extern_read_handler_.has_target())
    {
      extern_read_handler_.post(read_result_type(error, 0));
      return;
    }

    // Store error for the next outer read operation.
    read_error_ = error;
    return;
  }

  // We do not need in-between-frame-garbage and frame's head
  read_buffer_.consume(bytes_transferred);
  read_until_tail();
}

void cyclic_read_session::handle_read_tail(
    const boost::system::error_code& error,
    const std::size_t bytes_transferred)
{
  port_read_in_progress_ = false;

  // Check for pending session do_start_extern_stop operation
  if (extern_state::stop == extern_state_)
  {
    if (may_complete_stop())
    {
      complete_stop();
      post_extern_stop_handler();
    }
    return;
  }

  if (error)
  {
    // Check for pending session read operation
    if (extern_read_handler_.has_target())
    {
      extern_read_handler_.post(read_result_type(error, 0));
      return;
    }

    // Store error for the next outer read operation.
    read_error_ = error;
    return;
  }

  typedef boost::asio::streambuf::const_buffers_type        const_buffers_type;
  typedef boost::asio::buffers_iterator<const_buffers_type> buffers_iterator;
  // Extract frame from buffer to distinct memory area
  const_buffers_type committed_buffers(read_buffer_.data());
  buffers_iterator data_begin(buffers_iterator::begin(committed_buffers));
  buffers_iterator data_end(
      data_begin + bytes_transferred - frame_tail_.length());

  frame_ptr new_frame;
  if (frame_buffer_.full())
  {
    new_frame = frame_buffer_.front();
    new_frame->assign(data_begin, data_end);
  }
  else
  {
    new_frame = boost::make_shared<frame>(data_begin, data_end);
  }

  // Consume processed data
  read_buffer_.consume(bytes_transferred);

  // Continue inner operations loop.
  read_until_head();

  // Save ready frame into the cyclic read buffer
  frame_buffer_.push_back(new_frame);

  // If there is waiting read operation - complete it
  if (extern_read_handler_base* handler = extern_read_handler_.target())
  {
    read_result_type copy_result = handler->copy(frame_buffer_);
    frame_buffer_.erase_begin(copy_result.get<1>());
    extern_read_handler_.post(copy_result);
  }
}

void cyclic_read_session::post_extern_stop_handler()
{
  if (extern_stop_handler_.has_target())
  {
    // Signal shutdown completion
    extern_stop_handler_.post(stop_error_);
  }
}

} // namespace nmea
} // namespace ma
