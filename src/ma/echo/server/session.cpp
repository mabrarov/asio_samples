//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/assert.hpp>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session.hpp>

namespace ma {

namespace echo {

namespace server {

namespace {

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

class io_handler_binder
{
private:
  typedef io_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (session::*function_type)(const boost::system::error_code&, 
      const std::size_t);

  template <typename SessionPtr>
  io_handler_binder(function_type function, SessionPtr&& the_session)
    : function_(function)
    , session_(std::forward<SessionPtr>(the_session))
  {
  } 

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  io_handler_binder(this_type&& other)
    : function_(other.function_)
    , session_(std::move(other.session_))
  {
  }

  io_handler_binder(const this_type& other)
    : function_(other.function_)
    , session_(other.session_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  void operator()(const boost::system::error_code& error, 
      const std::size_t bytes_transferred)
  {
    ((*session_).*function_)(error, bytes_transferred);
  }

private:
  function_type function_;
  session_ptr   session_;
}; // class io_handler_binder

class timer_handler_binder
{
private:
  typedef timer_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (session::*function_type)(const boost::system::error_code&);

  template <typename SessionPtr>
  timer_handler_binder(function_type function, SessionPtr&& the_session)
    : function_(function)
    , session_(std::forward<SessionPtr>(the_session))
  {
  } 

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  timer_handler_binder(this_type&& other)
    : function_(other.function_)
    , session_(std::move(other.session_))
  {
  }

  timer_handler_binder(const this_type& other)
    : function_(other.function_)
    , session_(other.session_)
  {
  }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

  void operator()(const boost::system::error_code& error)
  {
    ((*session_).*function_)(error);
  }

private:
  function_type function_;
  session_ptr   session_;
}; // class timer_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // anonymous namespace

session::session(boost::asio::io_service& io_service, 
    const session_options& options)
  : socket_recv_buffer_size_(options.socket_recv_buffer_size())
  , socket_send_buffer_size_(options.socket_send_buffer_size())
  , no_delay_(options.no_delay())
  , inactivity_timeout_(options.inactivity_timeout())
  , external_state_(external_state::ready)
  , general_state_(general_state::work)
  , read_state_(read_state::wait)
  , write_state_(write_state::wait)
  , timer_state_(timer_state::ready)
  , timer_cancelled_(false)  
  , pending_operations_(0)  
  , io_service_(io_service)
  , strand_(io_service)
  , socket_(io_service)
  , timer_(io_service)
  , buffer_(options.buffer_size())
  , wait_handler_(io_service)
  , stop_handler_(io_service)                
{          
}      

void session::reset()
{
  external_state_     = external_state::ready;
  general_state_      = general_state::work;
  read_state_         = read_state::wait;
  write_state_        = write_state::wait;
  timer_state_        = timer_state::ready;
  timer_cancelled_    = false;
  pending_operations_ = 0;  
  wait_error_.clear();

  boost::system::error_code ignored;
  socket_.close(ignored);

  buffer_.reset();
}
              
boost::system::error_code session::start()
{                
  if (external_state::ready != external_state_)
  {
    return server_error::invalid_state;
  }

  if (boost::system::error_code error = apply_socket_options())
  {
    boost::system::error_code ignored;
    socket_.close(ignored);
    external_state_ = external_state::stopped;
    return error;
  }
  
  external_state_ = external_state::work;  
  continue_work();

  return boost::system::error_code();
}

boost::optional<boost::system::error_code> session::stop()
{        
  if ((external_state::stopped == external_state_)
      || (external_state::stop == external_state_))
  {          
    return boost::system::error_code(server_error::invalid_state);
  }
  
  external_state_ = external_state::stop;
  notify_work_completion(server_error::operation_aborted);
  
  if (general_state::work == general_state_)
  {
    begin_active_shutdown();
  }

  if (general_state::stopped == general_state_)
  {
    external_state_ = external_state::stopped;
    return boost::system::error_code();
  }

  return boost::optional<boost::system::error_code>();
}

boost::optional<boost::system::error_code> session::wait()
{                
  if ((external_state::work != external_state_)
      || wait_handler_.has_target())
  {
    return boost::system::error_code(server_error::invalid_state);
  }

  if (general_state::work != general_state_)
  {
    return wait_error_;
  }

  return boost::optional<boost::system::error_code>();
}      

void session::handle_read(const boost::system::error_code& error, 
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::stopped != general_state_, 
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "invalid read state");

  switch (general_state_)
  {
  case general_state::work:
    handle_read_at_work(error, bytes_transferred);
    break;

  case general_state::shutdown:
    handle_read_at_shutdown(error, bytes_transferred);
    break;

  case general_state::stop:
    handle_read_at_stop(error, bytes_transferred);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid general state");
    break;
  }
}

void session::handle_read_at_work(const boost::system::error_code& error, 
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::work == general_state_, 
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "invalid read state");

  --pending_operations_;
  read_state_ = read_state::wait;
  
  if (boost::system::error_code error = cancel_timer_activity())
  {
    read_state_ = read_state::stopped;
    begin_general_stop(error);
    return;
  }
  
  if (error && (boost::asio::error::eof != error))
  {
    read_state_ = read_state::stopped;
    begin_general_stop(error);
    return;
  }

  if (boost::asio::error::eof == error)
  {
    read_state_ = read_state::stopped;
  }

  buffer_.consume(bytes_transferred);
  continue_work();
}

void session::handle_read_at_shutdown(const boost::system::error_code& error, 
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_, 
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "invalid read state");

  --pending_operations_;
  read_state_ = read_state::wait;

  if (boost::system::error_code error = cancel_timer_activity())
  {
    read_state_ = read_state::stopped;
    begin_general_stop(error);
    return;
  }

  if (error && (boost::asio::error::eof != error))
  {
    read_state_ = read_state::stopped;
    begin_general_stop(error);
    return;
  }

  if (boost::asio::error::eof == error)
  {
    read_state_ = read_state::stopped;
  }
  
  buffer_.consume(bytes_transferred);
  continue_shutdown();
}

void session::handle_read_at_stop(const boost::system::error_code& /*error*/,
    std::size_t /*bytes_transferred*/)
{
  BOOST_ASSERT_MSG(general_state::stop == general_state_, 
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "invalid read state");

  --pending_operations_;
  read_state_ = read_state::stopped;

  continue_stop();
}

void session::handle_write(const boost::system::error_code& error, 
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::stopped != general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "invalid write state");

  switch (general_state_)
  {
  case general_state::work:
    handle_write_at_work(error, bytes_transferred);
    break;

  case general_state::shutdown:
    handle_write_at_shutdown(error, bytes_transferred);
    break;

  case general_state::stop:
    handle_write_at_stop(error, bytes_transferred);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid general state");
    break;
  }
}

void session::handle_write_at_work(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::work == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "invalid write state");

  --pending_operations_;
  write_state_ = write_state::wait;
  
  if (boost::system::error_code error = cancel_timer_activity())
  {
    write_state_ = write_state::stopped;
    begin_general_stop(error);
    return;
  }
  
  if (error)
  {
    write_state_ = write_state::stopped;    
    begin_general_stop(error);
    return;
  }

  buffer_.commit(bytes_transferred);
  continue_work();
}

void session::handle_write_at_shutdown(const boost::system::error_code& error,
    std::size_t bytes_transferred)
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "invalid write state");

  --pending_operations_;
  write_state_ = write_state::wait;

  if (boost::system::error_code error = cancel_timer_activity())
  {
    write_state_ = write_state::stopped;
    begin_general_stop(error);
    return;
  }

  if (error)
  {
    write_state_ = write_state::stopped;
    begin_general_stop(error);
    return;
  }

  buffer_.commit(bytes_transferred);
  continue_shutdown();
}

void session::handle_write_at_stop(const boost::system::error_code& /*error*/, 
    std::size_t /*bytes_transferred*/)
{
  BOOST_ASSERT_MSG(general_state::stop == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(write_state::in_progress == write_state_,
      "invalid write state");

  --pending_operations_;
  write_state_ = write_state::stopped;

  continue_stop();
}

void session::handle_timer(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(general_state::stopped != general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "invalid timer state");

  switch (general_state_)
  {
  case general_state::work:
    handle_timer_at_work(error);
    break;

  case general_state::shutdown:
    handle_timer_at_shutdown(error);
    break;

  case general_state::stop:
    handle_timer_at_stop(error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid general state");
    break;
  }
}

void session::handle_timer_at_work(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(general_state::work == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "invalid timer state");

  --pending_operations_;
  timer_state_ = timer_state::stopped;

  if (error && (boost::asio::error::operation_aborted != error))
  {    
    begin_general_stop(error);
    return;
  }

  if (timer_cancelled_)
  {
    timer_state_ = timer_state::ready;
    continue_work();
    return;
  }
  
  if (error)
  {
    begin_general_stop(error);
    return;
  }
  
  begin_general_stop(server_error::inactivity_timeout);
}

void session::handle_timer_at_shutdown(const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "invalid timer state");

  --pending_operations_;
  timer_state_ = timer_state::stopped;

  if (error && (boost::asio::error::operation_aborted != error))
  {
    begin_general_stop(error);
    return;
  }

  if (timer_cancelled_)
  {
    timer_state_ = timer_state::ready;
    continue_shutdown();
    return;
  }
  
  if (error)
  {    
    begin_general_stop(error);
    return;
  }

  begin_general_stop(server_error::inactivity_timeout);
}

void session::handle_timer_at_stop(const boost::system::error_code& /*error*/)
{
  BOOST_ASSERT_MSG(general_state::stop == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(timer_state::in_progress == timer_state_,
      "invalid timer state");

  --pending_operations_;
  timer_state_ = timer_state::stopped;

  continue_stop();
}

void session::continue_work()
{
  BOOST_ASSERT_MSG(general_state::work == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(write_state::stopped != write_state_,
      "invalid write state");

  BOOST_ASSERT_MSG(timer_state::stopped != timer_state_,
      "invalid timer state");

  if (read_state::stopped == read_state_)
  {    
    begin_passive_shutdown();
    return;
  }

  if (timer_state::in_progress == timer_state_)
  {
    return;
  }
  
  if (read_state::wait == read_state_)
  {
    cyclic_buffer::mutable_buffers_type read_buffers(buffer_.prepared());
    if (!read_buffers.empty())
    {
      // We have enough resources to start socket read
      begin_socket_read(read_buffers);
      ++pending_operations_;
      read_state_ = read_state::in_progress;
    }
  }

  if (write_state::wait == write_state_)
  {
    cyclic_buffer::const_buffers_type write_buffers(buffer_.data());
    if (!write_buffers.empty())
    {
      // We have enough resources to start socket write
      begin_socket_write(write_buffers);
      ++pending_operations_;
      write_state_ = write_state::in_progress;
    }
  }
  
  continue_timer_activity();  
}

void session::continue_timer_activity()
{
  BOOST_ASSERT_MSG(general_state::stopped != general_state_,
      "invalid general state");
    
  bool can_start_timer = timer_state::ready == timer_state_;

  bool has_activity = (read_state::in_progress == read_state_)
      || (write_state::in_progress == write_state_);

  if (can_start_timer && has_activity && inactivity_timeout_)
  {
    if (boost::system::error_code error = begin_timer_wait())
    {
      begin_general_stop(error);
      return;
    }
    ++pending_operations_;
    timer_state_ = timer_state::in_progress;
    timer_cancelled_ = false;
  }
}

boost::system::error_code session::cancel_timer_activity()
{
  boost::system::error_code error;
  if ((timer_state::in_progress == timer_state_) && !timer_cancelled_)
  {    
    timer_.cancel(error);
    timer_cancelled_ = true;
  }
  return error;
}

void session::continue_shutdown()
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");
  
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
    BOOST_ASSERT_MSG(false, "invalid read state");
    break;
  }  
}

void session::continue_shutdown_at_read_wait()
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::wait == read_state_,
      "invalid read state");

  if (write_state::wait == write_state_)  
  {  
    boost::system::error_code error;
    socket_.shutdown(protocol_type::socket::shutdown_send, error);
    if (error)
    {
      begin_general_stop(error);
      return;
    }
    write_state_ = write_state::stopped;    
  }

  if ((write_state::stopped == write_state_)
      && (timer_state::in_progress != timer_state_))
  {
    // We won't make any income data handling more
    buffer_.reset();
    cyclic_buffer::mutable_buffers_type read_buffers(buffer_.prepared());
    BOOST_ASSERT_MSG(!read_buffers.empty(), "buffer_ must be unfilled");

    // We have enough resources to start socket read
    begin_socket_read(read_buffers);
    ++pending_operations_;
    read_state_ = read_state::in_progress;
  } 

  continue_timer_activity();
}

void session::continue_shutdown_at_read_in_progress()
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::in_progress == read_state_,
      "invalid read state");

  if (write_state::wait == write_state_)
  {
    boost::system::error_code error;
    socket_.shutdown(protocol_type::socket::shutdown_send, error);
    if (error)
    {
      begin_general_stop(error);
      return;
    }
    write_state_ = write_state::stopped;
  }

  continue_timer_activity();
}

void session::continue_shutdown_at_read_stopped()
{
  BOOST_ASSERT_MSG(general_state::shutdown == general_state_,
      "invalid general state");

  BOOST_ASSERT_MSG(read_state::stopped == read_state_,
      "invalid read state");

  if (write_state::wait == write_state_)  
  {  
    boost::system::error_code error;
    socket_.shutdown(protocol_type::socket::shutdown_send, error);
    write_state_ = write_state::stopped;
    begin_general_stop(error);
    return;
  }

  if (write_state::stopped == write_state_)
  {
    begin_general_stop(boost::system::error_code());
    return;
  }
      
  continue_timer_activity();
}

void session::continue_stop()
{
  BOOST_ASSERT_MSG(general_state::stop == general_state_,
      "invalid general state");

  if (!pending_operations_)
  {    
    general_state_ = general_state::stopped;

    if (external_state::stop == external_state_)
    {
      external_state_ = external_state::stopped;    
      if (stop_handler_.has_target())
      {
        stop_handler_.post(boost::system::error_code());
      }
    }
  }
}

void session::begin_passive_shutdown()
{
  BOOST_ASSERT_MSG(general_state::work == general_state_,
      "invalid general state");

  general_state_ = general_state::shutdown; 

  if (external_state::work == external_state_)
  {
    notify_work_completion(server_error::passive_shutdown);
  }

  continue_shutdown();
}

void session::begin_active_shutdown()
{
  BOOST_ASSERT_MSG(general_state::work == general_state_,
      "invalid general state");

  general_state_ = general_state::shutdown;   

  continue_shutdown();
}

void session::begin_general_stop(boost::system::error_code error)
{
  BOOST_ASSERT_MSG((general_state::work == general_state_) 
      || (general_state::shutdown == general_state_),
      "invalid general state");
  
  boost::system::error_code close_error;
  socket_.close(close_error);
  if (!error)
  {
    error = close_error;
  }

  general_state_ = general_state::stop;

  if (external_state::work == external_state_)
  {
    notify_work_completion(error);
  }

  continue_stop();
}

void session::notify_work_completion(const boost::system::error_code& error)
{  
  if (!wait_error_)
  {
    wait_error_ = error;
  }
  if (wait_handler_.has_target())
  {
    wait_handler_.post(wait_error_);
  }
}

void session::begin_socket_read(
    const cyclic_buffer::mutable_buffers_type& buffers)
{
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_, 
      make_custom_alloc_handler(read_allocator_, io_handler_binder(
          &this_type::handle_read, shared_from_this()))));

#else

  socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_, 
      make_custom_alloc_handler(read_allocator_, boost::bind(
          &this_type::handle_read, shared_from_this(), 
          boost::asio::placeholders::error, 
          boost::asio::placeholders::bytes_transferred))));

#endif
}

void session::begin_socket_write(
    const cyclic_buffer::const_buffers_type& buffers)
{
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_, 
      make_custom_alloc_handler(write_allocator_, io_handler_binder(
          &this_type::handle_write, shared_from_this()))));

#else

  socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_, 
      make_custom_alloc_handler(write_allocator_, boost::bind(
          &this_type::handle_write, shared_from_this(), 
          boost::asio::placeholders::error, 
          boost::asio::placeholders::bytes_transferred))));

#endif
}

boost::system::error_code session::begin_timer_wait()
{
  boost::system::error_code error;
  timer_.expires_from_now(*inactivity_timeout_, error);

  if (!error)
  {
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    timer_.async_wait(MA_STRAND_WRAP(strand_, 
        make_custom_alloc_handler(inactivity_allocator_, timer_handler_binder(
            &this_type::handle_timer, shared_from_this()))));

#else

    timer_.async_wait(MA_STRAND_WRAP(strand_, 
        make_custom_alloc_handler(inactivity_allocator_, boost::bind(
            &this_type::handle_timer, shared_from_this(), 
            boost::asio::placeholders::error))));

#endif
  }

  return error;
}

boost::system::error_code session::apply_socket_options()
{
  typedef protocol_type::socket socket_type;
  
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

  if (socket_recv_buffer_size_)
  {
    boost::system::error_code error;
    socket_type::send_buffer_size opt(*socket_recv_buffer_size_);
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
