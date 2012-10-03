//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/ref.hpp>
#include <boost/assert.hpp>
#include <boost/make_shared.hpp>
#include <ma/config.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session.hpp>

namespace ma {
namespace echo {
namespace server {

namespace {

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

// Home-grown binders to support move semantic
class io_handler_binder
{
private:
  typedef io_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (session::*func_type)(const boost::system::error_code&,
      const std::size_t);

  template <typename SessionPtr>
  io_handler_binder(func_type func, SessionPtr&& session)
    : func_(func)
    , session_(std::forward<SessionPtr>(session))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  io_handler_binder(this_type&& other)
    : func_(other.func_)
    , session_(std::move(other.session_))
  {
  }

  io_handler_binder(const this_type& other)
    : func_(other.func_)
    , session_(other.session_)
  {
  }

#endif

  void operator()(const boost::system::error_code& error,
      const std::size_t bytes_transferred)
  {
    ((*session_).*func_)(error, bytes_transferred);
  }

private:
  func_type   func_;
  session_ptr session_;
}; // class io_handler_binder

class timer_handler_binder
{
private:
  typedef timer_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (session::*func_type)(const boost::system::error_code&);

  template <typename SessionPtr>
  timer_handler_binder(func_type func, SessionPtr&& session)
    : func_(func)
    , session_(std::forward<SessionPtr>(session))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  timer_handler_binder(this_type&& other)
    : func_(other.func_)
    , session_(std::move(other.session_))
  {
  }

  timer_handler_binder(const this_type& other)
    : func_(other.func_)
    , session_(other.session_)
  {
  }

#endif

  void operator()(const boost::system::error_code& error)
  {
    ((*session_).*func_)(error);
  }

private:
  func_type   func_;
  session_ptr session_;
}; // class timer_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // anonymous namespace

session_ptr session::create(boost::asio::io_service& io_service,
    const session_config& config)
{
  typedef shared_ptr_factory_helper<this_type> helper;
  return boost::make_shared<helper>(boost::ref(io_service), config);
}

session::session(boost::asio::io_service& io_service,
    const session_config& config)
  : max_transfer_size_(config.max_transfer_size)
  , socket_recv_buffer_size_(config.socket_recv_buffer_size)
  , socket_send_buffer_size_(config.socket_send_buffer_size)
  , no_delay_(config.no_delay)
  , inactivity_timeout_(to_optional_duration(config.inactivity_timeout))
  , extern_state_(extern_state::ready)
  , intern_state_(intern_state::work)
  , read_state_(read_state::wait)
  , write_state_(write_state::wait)
  , timer_state_(timer_state::ready)
  , timer_wait_cancelled_(false)
  , timer_turned_(false)
  , pending_operations_(0)
  , io_service_(io_service)
  , strand_(io_service)
  , socket_(io_service)
  , timer_(io_service)
  , buffer_(config.buffer_size)
  , extern_wait_handler_(io_service)
  , extern_stop_handler_(io_service)
{
}

void session::reset()
{
  // Reset state
  extern_state_ = extern_state::ready;
  intern_state_ = intern_state::work;
  read_state_   = read_state::wait;
  write_state_  = write_state::wait;
  timer_state_  = timer_state::ready;

  timer_wait_cancelled_ = false;
  timer_turned_         = false;
  pending_operations_   = 0;

  // reset() might be called right after connection was established
  // so we need to be sure that the socket will be closed.
  close_socket();

  // Post condition: filled sequence is empty, unfilled sequence is empty.
  buffer_.reset();
  extern_wait_error_.clear();
}

boost::system::error_code session::do_start_extern_start()
{
  // Check external state consistency
  if (extern_state::ready != extern_state_)
  {
    return server_error::invalid_state;
  }

  // Set up configured socket options
  if (boost::system::error_code error = apply_socket_options())
  {
    close_socket();
    // Switch states as SM suppose...
    extern_state_ = extern_state::stopped;
    intern_state_ = intern_state::stopped;
    read_state_   = read_state::stopped;
    write_state_  = write_state::stopped;
    timer_state_  = timer_state::stopped;
    // ... and notify start handler about error
    return error;
  }

  // Internal states have right values already
  extern_state_ = extern_state::work;
  continue_work();

  // Notify start handler about success
  return boost::system::error_code();
}

boost::optional<boost::system::error_code> session::do_start_extern_stop()
{
  // Check exetrnal state consistency
  if ((extern_state::stopped == extern_state_)
      || (extern_state::stop == extern_state_))
  {
    return boost::system::error_code(server_error::invalid_state);
  }

  // Switch external SM
  extern_state_ = extern_state::stop;
  complete_extern_wait(server_error::operation_aborted);

  if (intern_state::work == intern_state_)
  {
    start_active_shutdown();
  }

  // intern_state_ can be changed by start_active_shutdown
  if (intern_state::stopped == intern_state_)
  {
    extern_state_ = extern_state::stopped;
    // Notify stop handler about success
    return boost::system::error_code();
  }

  // Park stop handler for the late call
  return boost::optional<boost::system::error_code>();
}

boost::optional<boost::system::error_code> session::do_start_extern_wait()
{
  // Check exetrnal state consistency
  if ((extern_state::work != extern_state_)
      || extern_wait_handler_.has_target())
  {
    return boost::system::error_code(server_error::invalid_state);
  }

  if (intern_state::work != intern_state_)
  {
    // Notify wait handler about the happened stop
    return extern_wait_error_;
  }

  // Park wait handler for the late call
  return boost::optional<boost::system::error_code>();
}

void session::complete_extern_stop(const boost::system::error_code& error)
{
  if (extern_stop_handler_.has_target())
  {
    extern_stop_handler_.post(error);
  }
}

void session::complete_extern_wait(const boost::system::error_code& error)
{
  if (!extern_wait_error_)
  {
    extern_wait_error_ = error;
  }
  if (extern_wait_handler_.has_target())
  {
    extern_wait_handler_.post(extern_wait_error_);
  }
}

void session::handle_read(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "Invalid read state");

  // Split handler based on current internal state
  // that might change during read operation
  switch (intern_state_)
  {
  case intern_state::work:
    handle_read_at_work(error, bytes_transferred);
    break;

  case intern_state::shutdown:
    handle_read_at_shutdown(error, bytes_transferred);
    break;

  case intern_state::stop:
    handle_read_at_stop(error, bytes_transferred);
    break;

  default:
    BOOST_ASSERT_MSG(false, "Invalid internal state");
    break;
  }
}

void session::handle_read_at_work(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "Invalid read state");

  // Register operation completion and switch to the right state
  --pending_operations_;
  read_state_ = read_state::wait;

  // Try to cancel timer if it is in progress and wasn't already canceled
  if (boost::system::error_code error = cancel_timer_wait())
  {
    // Start session stop due to fatal error
    read_state_ = read_state::stopped;
    start_stop(error);
    return;
  }

  // If operation completed with error...
  if (error && (boost::asio::error::eof != error))
  {
    // ...start session stop due to fatal error
    read_state_ = read_state::stopped;
    start_stop(error);
    return;
  }

  // If EOF is recieved then read activity (SM) is stopped
  if (boost::asio::error::eof == error)
  {
    read_state_ = read_state::stopped;
  }

  // Handle read data
  buffer_.consume(bytes_transferred);
  continue_work();
}

void session::handle_read_at_shutdown(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "Invalid read state");

  // Register operation completion and switch to the right state
  --pending_operations_;
  read_state_ = read_state::wait;

  // Try to cancel timer if it is in progress and wasn't already canceled
  if (boost::system::error_code error = cancel_timer_wait())
  {
    read_state_ = read_state::stopped;
    start_stop(error);
    return;
  }

  if (error && (boost::asio::error::eof != error))
  {
    read_state_ = read_state::stopped;
    start_stop(error);
    return;
  }

  // If EOF is recieved then read activity is stopped
  if (boost::asio::error::eof == error)
  {
    read_state_ = read_state::stopped;
  }

  // Handle read data
  buffer_.consume(bytes_transferred);
  continue_shutdown();
}

void session::handle_read_at_stop(const boost::system::error_code& /*error*/,
    std::size_t /*bytes_transferred*/)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "Invalid read state");

  --pending_operations_;
  read_state_ = read_state::stopped;
  continue_stop();
}

void session::handle_write(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "Invalid write state");

  // Split handler based on current internal state
  // that might change during write operation
  switch (intern_state_)
  {
  case intern_state::work:
    handle_write_at_work(error, bytes_transferred);
    break;

  case intern_state::shutdown:
    handle_write_at_shutdown(error, bytes_transferred);
    break;

  case intern_state::stop:
    handle_write_at_stop(error, bytes_transferred);
    break;

  default:
    BOOST_ASSERT_MSG(false, "Invalid internal state");
    break;
  }
}

void session::handle_write_at_work(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "Invalid write state");

  --pending_operations_;
  write_state_ = write_state::wait;

  if (boost::system::error_code error = cancel_timer_wait())
  {
    write_state_ = write_state::stopped;
    start_stop(error);
    return;
  }

  // If operation completed with error...
  if (error)
  {
    // ...start session stop due to fatal error
    write_state_ = write_state::stopped;
    start_stop(error);
    return;
  }

  // Handle written data
  buffer_.commit(bytes_transferred);
  continue_work();
}

void session::handle_write_at_shutdown(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "Invalid write state");

  --pending_operations_;
  write_state_ = write_state::wait;

  // Try to cancel timer
  if (boost::system::error_code error = cancel_timer_wait())
  {
    write_state_ = write_state::stopped;
    start_stop(error);
    return;
  }

  if (error)
  {
    // Start session stop due to fatal error
    write_state_ = write_state::stopped;
    start_stop(error);
    return;
  }

  // Handle written data
  buffer_.commit(bytes_transferred);
  continue_shutdown();
}

void session::handle_write_at_stop(const boost::system::error_code& /*error*/,
    std::size_t /*bytes_transferred*/)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "Invalid write state");

  --pending_operations_;
  write_state_ = write_state::stopped;
  continue_stop();
}

void session::handle_timer(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "Invalid timer state");

  // Split handler based on current internal state
  // that might change during timer wait operation
  switch (intern_state_)
  {
  case intern_state::work:
  case intern_state::shutdown:
    handle_timer_at_work(error);
    break;

  case intern_state::stop:
    handle_timer_at_stop(error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "Invalid internal state");
    break;
  }
}

void session::handle_timer_at_work(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG((intern_state::work == intern_state_)
      || (intern_state::shutdown == intern_state_),
      "Invalid internal state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "Invalid timer state");

  --pending_operations_;
  timer_state_ = timer_state::ready;

  if (error && (boost::asio::error::operation_aborted != error))
  {
    // Start session stop due to fatal error
    timer_state_ = timer_state::stopped;
    start_stop(error);
    return;
  }

  if (timer_wait_cancelled_)
  {
    // Continue normal workflow
    if (timer_turned_)
    {
      start_timer_wait();
    }
    return;
  }

  // Start session stop due to client inactivity timeout
  timer_state_ = timer_state::stopped;
  start_stop(server_error::inactivity_timeout);
}

void session::handle_timer_at_stop(const boost::system::error_code& /*error*/)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "Invalid timer state");

  --pending_operations_;
  timer_state_ = timer_state::stopped;
  continue_stop();
}

void session::continue_work()
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(write_state::stopped != write_state_,
      "Invalid write state");

  BOOST_ASSERT_MSG(timer_state::stopped != timer_state_,
      "Invalid timer state");

  // Check for active shutdown start
  if (read_state::stopped == read_state_)
  {
    start_passive_shutdown();
    return;
  }

  if (read_state::wait == read_state_)
  {
    cyclic_buffer::mutable_buffers_type read_buffers(
        buffer_.prepared(max_transfer_size_));
    if (!read_buffers.empty())
    {
      // We have enough resources to begin socket read
      start_socket_read(read_buffers);
    }
  }

  if (write_state::wait == write_state_)
  {
    cyclic_buffer::const_buffers_type write_buffers(
        buffer_.data(max_transfer_size_));
    if (!write_buffers.empty())
    {
      // We have enough resources to begin socket write
      start_socket_write(write_buffers);
    }
  }

  // Turn on timer if need
  continue_timer_wait();
}

void session::continue_timer_wait()
{
  BOOST_ASSERT_MSG(intern_state::stopped != intern_state_,
      "Invalid internal state");

  if (inactivity_timeout_)
  {
    bool has_io_activity = (read_state::in_progress == read_state_)
        || (write_state::in_progress == write_state_);
    if (has_io_activity && !timer_turned_)
    {
      // Update timer expiry
      boost::system::error_code error;
      timer_.expires_from_now(*inactivity_timeout_, error);
      if (error)
      {
        start_stop(error);
        return;
      }

      timer_wait_cancelled_ = true;
      timer_turned_         = true;

      // Start async wait if it can be done right now.
      // Otherwise it will be done by handle_timer.
      if (timer_state::ready == timer_state_)
      {
        start_timer_wait();
      }
    }
  }
}

void session::continue_shutdown()
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  // Split control flow based on current state of read activity to simplify
  switch (read_state_)
  {
  case read_state::wait:
    continue_shutdown_at_read_wait();
    break;

  case read_state::in_progress:
    continue_shutdown_at_read_in_progress();
    break;

  case read_state::stopped:
    continue_shutdown_at_read_stopped();
    break;

  default:
    BOOST_ASSERT_MSG(false, "Invalid read state");
    break;
  }
}

void session::continue_shutdown_at_read_wait()
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::wait == read_state_,
      "Invalid read state");

  if (write_state::wait == write_state_)
  {
    // We can shutdown outgoing part of TCP stream
    if (boost::system::error_code error = shutdown_socket())
    {
      // Fatal error
      start_stop(error);
      return;
    }
    write_state_ = write_state::stopped;
  }

  if (write_state::stopped == write_state_)
  {
    // We won't make any income data handling more
    buffer_.reset();
    cyclic_buffer::mutable_buffers_type read_buffers(buffer_.prepared());
    BOOST_ASSERT_MSG(!read_buffers.empty(), "buffer_ must be unfilled");

    // We have enough resources to begin socket read
    start_socket_read(read_buffers);
  }
  else
  {
    // write_state::in_progress == write_state_
    cyclic_buffer::mutable_buffers_type read_buffers(buffer_.prepared());
    if (!read_buffers.empty())
    {
      // We have enough resources to begin socket read
      start_socket_read(read_buffers);
    }
  }

  // Turn on timer if need
  continue_timer_wait();
}

void session::continue_shutdown_at_read_in_progress()
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "Invalid read state");

  if (write_state::wait == write_state_)
  {
    // We can shutdown outgoing part of TCP stream
    if (boost::system::error_code error = shutdown_socket())
    {
      // Fatal error
      start_stop(error);
      return;
    }
    write_state_ = write_state::stopped;
  }

  // Turn on timer if need
  continue_timer_wait();
}

void session::continue_shutdown_at_read_stopped()
{
  BOOST_ASSERT_MSG(intern_state::shutdown == intern_state_,
      "Invalid internal state");

  BOOST_ASSERT_MSG(read_state::stopped == read_state_,
      "Invalid read state");

  if (write_state::wait == write_state_)
  {
    // We can shutdown outgoing part of TCP stream
    shutdown_socket();
    // Shutdown error has be ignored because read activity is already stopped
    write_state_ = write_state::stopped;
  }

  if (write_state::stopped == write_state_)
  {
    // Read and write activities are stopped,
    // so we can begin normal (unrelated to any error) internal general stop
    start_stop(boost::system::error_code());
    return;
  }

  // Turn on timer if need
  continue_timer_wait();
}

void session::continue_stop()
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_,
      "Invalid internal state");

  if (!pending_operations_)
  {
    BOOST_ASSERT_MSG(read_state::stopped  == read_state_,
        "Invalid read state");

    BOOST_ASSERT_MSG(write_state::stopped == write_state_,
        "Invalid write state");

    BOOST_ASSERT_MSG(timer_state::stopped == timer_state_,
        "Invalid timer state");

    // Internal general stop completed
    intern_state_ = intern_state::stopped;

    if (extern_state::stop == extern_state_)
    {
      extern_state_ = extern_state::stopped;
      complete_extern_stop(boost::system::error_code());
    }
  }
}

void session::start_passive_shutdown()
{
  start_shutdown(server_error::out_of_work);
}

void session::start_active_shutdown()
{
  start_shutdown(server_error::operation_aborted);
}

void session::start_shutdown(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_,
      "Invalid internal state");

  // Siwtch general internal SM
  intern_state_ = intern_state::shutdown;

  // Notify external wait handler if need
  if (extern_state::work == extern_state_)
  {
    complete_extern_wait(error);
  }

  continue_shutdown();
}

void session::start_stop(boost::system::error_code error)
{
  BOOST_ASSERT_MSG((intern_state::work == intern_state_)
      || (intern_state::shutdown == intern_state_),
      "Invalid internal state");

  // Siwtch general internal SM
  intern_state_ = intern_state::stop;

  // Close the socket and register error if there was no stop error before
  if (boost::system::error_code close_error = close_socket())
  {
    if (!error)
    {
      error = close_error;
    }
  }

  // Stop timer and register error if there was no stop error before
  if (boost::system::error_code timer_error = cancel_timer_wait())
  {
    if (!error)
    {
      error = timer_error;
    }
  }

  // Stop all internal SMs (activities) that are already ready to stop
  if (read_state::wait == read_state_)
  {
    read_state_ = read_state::stopped;
  }
  if (write_state::wait == write_state_)
  {
    write_state_ = write_state::stopped;
  }
  if (timer_state::ready == timer_state_)
  {
    timer_state_ = timer_state::stopped;
  }

  // Notify wait handler if need
  if (extern_state::work == extern_state_)
  {
    complete_extern_wait(error);
  }

  continue_stop();
}

void session::start_socket_read(
    const cyclic_buffer::mutable_buffers_type& buffers)
{
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(read_allocator_, io_handler_binder(
          &this_type::handle_read, shared_from_this()))));

#else

  socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(read_allocator_, boost::bind(
          &this_type::handle_read, shared_from_this(), _1, _2))));

#endif

  ++pending_operations_;
  read_state_ = read_state::in_progress;
}

void session::start_socket_write(
    const cyclic_buffer::const_buffers_type& buffers)
{
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(write_allocator_, io_handler_binder(
          &this_type::handle_write, shared_from_this()))));

#else

  socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(write_allocator_, boost::bind(
          &this_type::handle_write, shared_from_this(), _1, _2))));

#endif

  ++pending_operations_;
  write_state_ = write_state::in_progress;
}

void session::start_timer_wait()
{
  BOOST_ASSERT_MSG(timer_state::ready == timer_state_,
      "Invalid timer state");

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  timer_.async_wait(MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(timer_allocator_, timer_handler_binder(
          &this_type::handle_timer, shared_from_this()))));

#else

  timer_.async_wait(MA_STRAND_WRAP(strand_,
      make_custom_alloc_handler(timer_allocator_, boost::bind(
          &this_type::handle_timer, shared_from_this(), _1))));

#endif

  ++pending_operations_;
  timer_state_ = timer_state::in_progress;
  timer_wait_cancelled_ = false;
}

boost::system::error_code session::cancel_timer_wait()
{
  boost::system::error_code error;
  // Cancellation of timer can be rather heavy so do it once
  if (!timer_wait_cancelled_ && (timer_state::in_progress == timer_state_))
  {
    timer_.cancel(error);
  }
  if (!error)
  {
    timer_wait_cancelled_ = true;
    timer_turned_         = false;
  }
  return error;
}

boost::system::error_code session::shutdown_socket()
{
  boost::system::error_code error;
  socket_.shutdown(protocol_type::socket::shutdown_send, error);
  return error;
}

boost::system::error_code session::close_socket()
{
  boost::system::error_code error;
  socket_.close(error);
  return error;
}

boost::system::error_code session::apply_socket_options()
{
  typedef protocol_type::socket socket_type;

  // Setup abortive shutdown sequence for closesocket
  {
    boost::system::error_code error;
    socket_type::linger opt(false, 0);
    socket_.set_option(opt, error);
    if (error)
    {
      return error;
    }
  }

  // Apply all (really) configered socket options
  if (socket_recv_buffer_size_)
  {
    boost::system::error_code error;
    socket_type::receive_buffer_size opt(*socket_recv_buffer_size_);
    socket_.set_option(opt, error);
    if (error)
    {
      return error;
    }
  }

  if (socket_send_buffer_size_)
  {
    boost::system::error_code error;
    socket_type::send_buffer_size opt(*socket_send_buffer_size_);
    socket_.set_option(opt, error);
    if (error)
    {
      return error;
    }
  }

  if (no_delay_)
  {
    boost::system::error_code error;
    protocol_type::no_delay opt(*no_delay_);
    socket_.set_option(opt, error);
    if (error)
    {
      return error;
    }
  }

  return boost::system::error_code();
}

} // namespace server
} // namespace echo
} // namespace ma
