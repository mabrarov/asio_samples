//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READ_SESSION_HPP
#define MA_NMEA_CYCLIC_READ_SESSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <string>
#include <utility>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/next_prior.hpp>
#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_handler.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/handler_cont_helpers.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/detail/tuple.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/utility.hpp>
#include "frame.hpp"
#include "error.hpp"
#include "cyclic_read_session_fwd.hpp"

namespace ma {
namespace nmea {

class cyclic_read_session;
typedef detail::shared_ptr<cyclic_read_session> cyclic_read_session_ptr;

class cyclic_read_session
  : private boost::noncopyable
  , public  detail::enable_shared_from_this<cyclic_read_session>
{
private:
  typedef cyclic_read_session this_type;
  enum { max_message_size = 512 };

public:
  enum { min_read_buffer_size = max_message_size };
  enum { min_message_queue_size = 1 };

  static cyclic_read_session_ptr create(boost::asio::io_service& io_service,
      std::size_t read_buffer_size, std::size_t frame_buffer_size,
      const std::string& frame_head, const std::string& frame_tail);

  boost::asio::serial_port& serial_port();
  void resest();

  template <typename Handler>
  void async_start(MA_FWD_REF(Handler) handler);

  template <typename Handler>
  void async_stop(MA_FWD_REF(Handler) handler);

  // Handler()(const boost::system::error_code&, std::size_t)
  template <typename Handler, typename Iterator>
  void async_read_some(Iterator begin, Iterator end, 
      MA_FWD_REF(Handler) handler);

  template <typename ConstBufferSequence, typename Handler>
  void async_write_some(ConstBufferSequence buffers,
      MA_FWD_REF(Handler) handler);

protected:
  cyclic_read_session(boost::asio::io_service& io_service,
      std::size_t read_buffer_size, std::size_t frame_buffer_size,
      const std::string& frame_head, const std::string& frame_tail);

  ~cyclic_read_session();

private:
  struct extern_state
  {
    enum value_t {ready, work, stop, stopped};
  }; // struct ext_state

  typedef boost::optional<boost::system::error_code> optional_error_code;
  typedef boost::circular_buffer<frame_ptr> frame_buffer_type;
  typedef detail::tuple<boost::system::error_code, std::size_t>
      read_result_type;

  template <typename Iterator>
  static read_result_type copy_buffer(const frame_buffer_type& buffer,
      const Iterator& begin, const Iterator& end);

  class extern_read_handler_base
  {
  private:
    typedef extern_read_handler_base this_type;

  public:
    typedef read_result_type (*copy_func_type)(
        extern_read_handler_base*, const frame_buffer_type&);

    explicit extern_read_handler_base(copy_func_type copy_func);

    read_result_type copy(const frame_buffer_type& buffer);

  private:
    copy_func_type copy_func_;
  }; // class extern_read_handler_base

  template <typename Handler, typename Iterator>
  class wrapped_extern_read_handler;

  template <typename Handler>
  void start_extern_start(Handler&);

  template <typename Handler>
  void start_extern_stop(Handler&);

  template <typename Handler, typename Iterator>
  void start_extern_read_some(Iterator&, Iterator&, Handler&);

  template <typename ConstBufferSequence, typename Handler>
  void start_extern_write_some(ConstBufferSequence&, Handler&);

  boost::system::error_code do_start_extern_start();
  optional_error_code do_start_extern_stop();
  optional_error_code do_start_extern_read_some();

  bool may_complete_stop() const;
  void complete_stop();

  template <typename Handler>
  void handle_write(const boost::system::error_code& error,
      std::size_t bytes_transferred, const detail::tuple<Handler>& handler);

  void read_until_head();
  void read_until_tail();
  void handle_read_head(const boost::system::error_code& error,
      const std::size_t bytes_transferred);
  void handle_read_tail(const boost::system::error_code& error,
      const std::size_t bytes_transferred);
  void post_extern_stop_handler();

  boost::asio::io_service&        io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::serial_port        serial_port_;

  ma::handler_storage<read_result_type, extern_read_handler_base>
      extern_read_handler_;
  ma::handler_storage<boost::system::error_code> extern_stop_handler_;

  frame_buffer_type         frame_buffer_;
  boost::system::error_code read_error_;
  boost::system::error_code stop_error_;

  bool port_write_in_progress_;
  bool port_read_in_progress_;
  extern_state::value_t extern_state_;

  boost::asio::streambuf read_buffer_;
  const std::string      frame_head_;
  const std::string      frame_tail_;

  in_place_handler_allocator<256> write_allocator_;
  in_place_handler_allocator<256> read_allocator_;
}; // class cyclic_read_session

template <typename Handler, typename Iterator>
class cyclic_read_session::wrapped_extern_read_handler
  : public extern_read_handler_base
{
private:
  typedef wrapped_extern_read_handler<Handler, Iterator> this_type;

public:

  template <typename H, typename I>
  wrapped_extern_read_handler(MA_FWD_REF(H) handler,
      MA_FWD_REF(I) begin, MA_FWD_REF(I) end);

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  wrapped_extern_read_handler(this_type&&);
  wrapped_extern_read_handler(const this_type&);

#endif

  static read_result_type do_copy(extern_read_handler_base* base,
      const frame_buffer_type& buffer);

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

  void operator()(const read_result_type& result);

private:
  Handler  handler_;
  Iterator start_;
  Iterator end_;
}; // class cyclic_read_session::wrapped_extern_read_handler

inline boost::asio::serial_port& cyclic_read_session::serial_port()
{
  return serial_port_;
}

template <typename Handler>
void cyclic_read_session::async_start(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef void (this_type::*func_type)(handler_type&);

  func_type func = &this_type::start_extern_start<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(func, shared_from_this(), detail::placeholders::_1)));
}

template <typename Handler>
void cyclic_read_session::async_stop(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef void (this_type::*func_type)(handler_type&);

  func_type func = &this_type::start_extern_stop<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(func, shared_from_this(), detail::placeholders::_1)));
}

// Handler()(const boost::system::error_code&, std::size_t)
template <typename Handler, typename Iterator>
void cyclic_read_session::async_read_some(
    Iterator begin, Iterator end, MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Iterator>::type iterator_type;
  typedef typename detail::decay<Handler>::type  handler_type;
  typedef void (this_type::*func_type)(iterator_type&, iterator_type&,
      handler_type&);

  func_type func =
      &this_type::start_extern_read_some<handler_type, iterator_type>;

  strand_.post(make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(func, shared_from_this(), detail::forward<Iterator>(begin),
          detail::forward<Iterator>(end), detail::placeholders::_1)));
}

template <typename ConstBufferSequence, typename Handler>
void cyclic_read_session::async_write_some(
    ConstBufferSequence buffers, MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<ConstBufferSequence>::type
      buffers_type;
  typedef typename detail::decay<Handler>::type handler_type;
  typedef void (this_type::*func_type)(buffers_type&, handler_type&);

  func_type func =
      &this_type::start_extern_write_some<buffers_type, handler_type>;

  strand_.post(make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(func, shared_from_this(),
          detail::forward<ConstBufferSequence>(buffers),
          detail::placeholders::_1)));
}

inline cyclic_read_session::~cyclic_read_session()
{
}

template <typename Iterator>
cyclic_read_session::read_result_type cyclic_read_session::copy_buffer(
    const frame_buffer_type& buffer, const Iterator& begin, const Iterator& end)
{
  std::size_t copy_size = std::min<std::size_t>(
      std::distance(begin, end), buffer.size());

  typedef frame_buffer_type::const_iterator buffer_iterator;

  buffer_iterator src_begin = buffer.begin();
  buffer_iterator src_end = boost::next(src_begin, copy_size);
  std::copy(src_begin, src_end, begin);

  return read_result_type(boost::system::error_code(), copy_size);
}

inline cyclic_read_session::extern_read_handler_base::extern_read_handler_base(
    copy_func_type copy_func)
  : copy_func_(copy_func)
{
}

inline cyclic_read_session::read_result_type
cyclic_read_session::extern_read_handler_base::copy(
    const frame_buffer_type& buffer)
{
  return copy_func_(this, buffer);
}

template <typename Handler, typename Iterator>
template <typename H, typename I>
cyclic_read_session::wrapped_extern_read_handler<Handler, Iterator>
    ::wrapped_extern_read_handler(MA_FWD_REF(H) handler, MA_FWD_REF(I) begin,
          MA_FWD_REF(I) end)
  : extern_read_handler_base(&this_type::do_copy)
  , handler_(detail::forward<H>(handler))
  , start_(detail::forward<I>(begin))
  , end_(detail::forward<I>(end))
{
}

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

template <typename Handler, typename Iterator>
cyclic_read_session::wrapped_extern_read_handler<Handler, Iterator>
    ::wrapped_extern_read_handler(this_type&& other)
  : extern_read_handler_base(detail::move(other))
  , handler_(detail::move(other.handler_))
  , start_(detail::move(other.start_))
  , end_(detail::move(other.end_))
{
}

template <typename Handler, typename Iterator>
cyclic_read_session::wrapped_extern_read_handler<Handler, Iterator>
  ::wrapped_extern_read_handler(const this_type& other)
  : extern_read_handler_base(other)
  , handler_(other.handler_)
  , start_(other.start_)
  , end_(other.end_)
{
}

#endif

template <typename Handler, typename Iterator>
cyclic_read_session::read_result_type
cyclic_read_session::wrapped_extern_read_handler<Handler, Iterator>::do_copy(
    extern_read_handler_base* base, const frame_buffer_type& buffer)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  return copy_buffer(buffer, this_ptr->start_, this_ptr->end_);
}

template <typename Handler, typename Iterator>
void cyclic_read_session::wrapped_extern_read_handler<Handler, Iterator>
    ::operator()(const read_result_type& result)
{
  handler_(detail::get<0>(result), detail::get<1>(result));
}

template <typename Handler>
void cyclic_read_session::start_extern_start(Handler& handler)
{
  boost::system::error_code error = do_start_extern_start();
  io_service_.post(bind_handler(detail::move(handler), error));
}

template <typename Handler>
void cyclic_read_session::start_extern_stop(Handler& handler)
{
  if (optional_error_code result = do_start_extern_stop())
  {
    io_service_.post(bind_handler(detail::move(handler), *result));
  }
  else
  {
    extern_stop_handler_.store(detail::move(handler));
  }
}

template <typename Handler, typename Iterator>
void cyclic_read_session::start_extern_read_some(
    Iterator& begin, Iterator& end, Handler& handler)
{
  if (optional_error_code read_result = do_start_extern_read_some())
  {
    // Complete read operation "in place" if error
    if (*read_result)
    {
      io_service_.post(bind_handler(detail::move(handler), *read_result, 0));
      return;
    }

    // Try to copy buffer data
    read_result_type copy_result = copy_buffer(frame_buffer_, begin, end);
    frame_buffer_.erase_begin(detail::get<1>(copy_result));

    // Post the handler
    io_service_.post(bind_handler(detail::move(handler),
        detail::get<0>(copy_result), detail::get<1>(copy_result)));
  }
  else
  {
    typedef wrapped_extern_read_handler<Handler, Iterator> wrapped_handler_type;
    extern_read_handler_.store(wrapped_handler_type(
        detail::move(handler), detail::move(begin), detail::move(end)));
  }
}

template <typename ConstBufferSequence, typename Handler>
void cyclic_read_session::start_extern_write_some(ConstBufferSequence& buffers,
    Handler& handler)
{
  if ((extern_state::work != extern_state_) || port_write_in_progress_)
  {
    boost::system::error_code error(nmea::error::invalid_state);
    io_service_.post(bind_handler(detail::move(handler), error, 0));
    return;
  }

  serial_port_.async_write_some(detail::move(buffers), strand_.wrap(
      make_custom_alloc_handler(write_allocator_, detail::bind(
          &this_type::handle_write<Handler>, shared_from_this(),
          detail::placeholders::_1, detail::placeholders::_2,
          detail::make_tuple<Handler>(detail::move(handler))))));

  port_write_in_progress_ = true;
}

template <typename Handler>
void cyclic_read_session::handle_write(const boost::system::error_code& error,
    std::size_t bytes_transferred, const detail::tuple<Handler>& handler)
{
  port_write_in_progress_ = false;

  io_service_.post(bind_handler(
      detail::get<0>(handler), error, bytes_transferred));

  if ((extern_state::stop == extern_state_) && may_complete_stop())
  {
    complete_stop();
    post_extern_stop_handler();
  }
}

} // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_HPP
