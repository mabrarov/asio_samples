//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READ_SESSION_HPP
#define MA_NMEA_CYCLIC_READ_SESSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <utility>
#include <algorithm>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/next_prior.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/nmea/frame.hpp>
#include <ma/nmea/error.hpp>
#include <ma/nmea/cyclic_read_session_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace nmea {

class cyclic_read_session;
typedef boost::shared_ptr<cyclic_read_session> cyclic_read_session_ptr;

class cyclic_read_session 
  : private boost::noncopyable
  , public boost::enable_shared_from_this<cyclic_read_session>
{
private:
  typedef cyclic_read_session this_type;      
  enum { max_message_size = 512 };

public:
  enum { min_read_buffer_size = max_message_size };
  enum { min_message_queue_size = 1 };

  cyclic_read_session(boost::asio::io_service& io_service, 
      const std::size_t read_buffer_size, const std::size_t frame_buffer_size,
      const std::string& frame_head, const std::string& frame_tail);

  ~cyclic_read_session()
  {          
  }

  boost::asio::serial_port& serial_port()
  {
    return serial_port_;
  }

  void resest();

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_start<handler_type>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),  
        boost::bind(&this_type::begin_stop<handler_type>, 
            shared_from_this(), _1)));
  }

  // Handler()(const boost::system::error_code&, std::size_t)
  template <typename Handler, typename Iterator>
  void async_read_some(Iterator&& begin, Iterator&& end, Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Iterator>::type iterator_type;
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),  
        boost::bind(&this_type::begin_read_some<handler_type, iterator_type>, 
            shared_from_this(), std::forward<Iterator>(begin), 
            std::forward<Iterator>(end), _1)));
  }

  template <typename ConstBufferSequence, typename Handler>
  void async_write(ConstBufferSequence&& buffers, Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<ConstBufferSequence>::type buffers_type;
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_write<buffers_type, handler_type>, 
            shared_from_this(), std::forward<ConstBufferSequence>(buffers), 
            _1)));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(&this_type::begin_start<Handler>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(&this_type::begin_stop<Handler>, shared_from_this(), _1)));
  }

  // Handler()(const boost::system::error_code&, std::size_t)
  template <typename Handler, typename Iterator>
  void async_read_some(Iterator begin, Iterator end, const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(&this_type::begin_read_some<Handler, Iterator>, 
            shared_from_this(), begin, end, _1)));
  }

  template <typename ConstBufferSequence, typename Handler>
  void async_write(ConstBufferSequence buffers, const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_write<ConstBufferSequence, Handler>, 
            shared_from_this(), buffers, _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)
    
private:
  enum external_state_type
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  }; // enum external_state_type

  typedef boost::circular_buffer<frame_ptr> frame_buffer_type;  

  typedef boost::tuple<boost::system::error_code, std::size_t> 
      read_result_type;

  template <typename Iterator>
  static read_result_type copy_buffer(const frame_buffer_type& buffer, 
      const Iterator& begin, const Iterator& end)
  {        
    std::size_t copy_size = std::min<std::size_t>(
        std::distance(begin, end), buffer.size());

    typedef frame_buffer_type::const_iterator buffer_iterator;

    buffer_iterator src_begin = buffer.begin();
    buffer_iterator src_end = boost::next(src_begin, copy_size);
    std::copy(src_begin, src_end, begin);
    
    return read_result_type(boost::system::error_code(), copy_size);
  }

  class read_handler_base
  {
  private:
    typedef read_handler_base this_type;
    this_type& operator=(const this_type&);

  public:
    typedef read_result_type (*copy_func_type)(
        read_handler_base*, const frame_buffer_type&);

    explicit read_handler_base(copy_func_type copy_func)
      : copy_func_(copy_func)          
    {
    }

    read_result_type copy(const frame_buffer_type& buffer)
    {
      return copy_func_(this, buffer);
    }
        
  private:
    copy_func_type copy_func_;        
  }; // class read_handler_base  

  template <typename Handler, typename Iterator>
  class wrapped_read_handler : public read_handler_base
  {
  private:
    typedef wrapped_read_handler<Handler, Iterator> this_type;
    this_type& operator=(const this_type&);

  public:

#if defined(MA_HAS_RVALUE_REFS)

    template <typename H, typename I>
    wrapped_read_handler(H&& handler, I&& begin, I&& end)
      : read_handler_base(&this_type::do_copy)
      , handler_(std::forward<H>(handler))
      , begin_(std::forward<I>(begin))
      , end_(std::forward<I>(end))
    {
    }

    wrapped_read_handler(this_type&& other)
      : read_handler_base(std::move(other))
      , handler_(std::move(other.handler_))
      , begin_(std::move(other.begin_))
      , end_(std::move(other.end_))
    {
    }

#else // defined(MA_HAS_RVALUE_REFS)

    wrapped_read_handler(const Handler& handler, 
        const Iterator& begin, const Iterator& end)
      : read_handler_base(&this_type::do_copy)
      , handler_(handler)
      , begin_(begin)
      , end_(end)
    {
    }

#endif // defined(MA_HAS_RVALUE_REFS)

    ~wrapped_read_handler()
    {
    }

    static read_result_type do_copy(read_handler_base* base, 
        const frame_buffer_type& buffer)
    {
      this_type* this_ptr = static_cast<this_type*>(base);          
      return copy_buffer(buffer, this_ptr->begin_, this_ptr->end_);         
    }

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

    template <typename Function>
    friend void asio_handler_invoke(const Function& function, 
        this_type* context)
    {
      ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
    }

    void operator()(const read_result_type& result)
    {
      handler_(result.get<0>(), result.get<1>());
    }

  private:
    Handler  handler_;
    Iterator begin_;
    Iterator end_;
  }; // class wrapped_read_handler

  template <typename Handler>
  void begin_start(const Handler& handler)
  {
    boost::system::error_code error = start();
    io_service_.post(detail::bind_handler(handler, error));
  }

  template <typename Handler>
  void begin_stop(const Handler& handler)
  { 
    if (boost::optional<boost::system::error_code> result = stop())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      stop_handler_.put(handler);
    }
  }

  template <typename Handler, typename Iterator>
  void begin_read_some(const Iterator& begin, const Iterator& end, 
      const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> read_result = read_some())
    { 
      // Complete read operation "in place" if error
      if (*read_result)
      {            
        io_service_.post(detail::bind_handler(handler, *read_result, 0));
        return;
      }

      // Try to copy buffer data
      read_result_type copy_result = copy_buffer(frame_buffer_, begin, end);
      frame_buffer_.erase_begin(copy_result.get<1>());

      // Post the handler
      io_service_.post(detail::bind_handler(
          handler, copy_result.get<0>(), copy_result.get<1>()));

      return;
    }
    put_read_handler(begin, end, handler);
  }

  template <typename Handler, typename Iterator>
  void put_read_handler(const Iterator& begin, const Iterator& end, 
      const Handler& handler)
  {
    typedef wrapped_read_handler<Handler, Iterator> wrapped_handler_type;

    wrapped_handler_type wrapped_handler(handler, begin, end);

    wrapped_handler_type* wrapped_handler_ptr = 
        boost::addressof(wrapped_handler);
    read_handler_base* base_handler_ptr = 
        static_cast<read_handler_base*>(wrapped_handler_ptr);

    read_handler_base_shift_ = reinterpret_cast<char*>(base_handler_ptr) 
        - reinterpret_cast<char*>(wrapped_handler_ptr);
    read_handler_.put(wrapped_handler);
  }

  read_handler_base* get_read_handler() const
  {
    return reinterpret_cast<read_handler_base*>(
        reinterpret_cast<char*>(read_handler_.target()) 
            + read_handler_base_shift_);
  }

  template <typename ConstBufferSequence, typename Handler>
  void begin_write(const ConstBufferSequence& buffers, const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result = 
        write(buffers, handler))
    {          
      io_service_.post(detail::bind_handler(handler, *result));
    }
  }

  boost::system::error_code                  start();
  boost::optional<boost::system::error_code> stop();
  boost::optional<boost::system::error_code> read_some();
  bool may_complete_stop() const;
  void complete_stop();

  template <typename ConstBufferSequence, typename Handler>
  boost::optional<boost::system::error_code> write(
      const ConstBufferSequence& buffers, const Handler& handler)
  {
    if ((started != external_state_) || port_write_in_progress_)
    {                             
      return session_error::invalid_state;
    }

    boost::asio::async_write(serial_port_, buffers, MA_STRAND_WRAP(strand_, 
        make_custom_alloc_handler(write_allocator_, boost::bind(
            &this_type::handle_write<Handler>, shared_from_this(), 
            boost::asio::placeholders::error, 
            boost::make_tuple<Handler>(handler)))));

    port_write_in_progress_ = true;

    return boost::optional<boost::system::error_code>();
  }
     
  template <typename Handler>
  void handle_write(const boost::system::error_code& error, 
      const boost::tuple<Handler>& handler)
  {         
    port_write_in_progress_ = false;

    io_service_.post(detail::bind_handler(boost::get<0>(handler), error));

    if ((stop_in_progress == external_state_) && !port_read_in_progress_)
    {
      external_state_ = stopped;
      post_stop_handler();
    }
  }

  void read_until_head();
  void read_until_tail();
  void handle_read_head(const boost::system::error_code& error, const std::size_t bytes_transferred);
  void handle_read_tail(const boost::system::error_code& error, const std::size_t bytes_transferred);      
  void post_stop_handler();
      
  boost::asio::io_service&        io_service_;
  boost::asio::io_service::strand strand_;
  boost::asio::serial_port        serial_port_;      

  ma::handler_storage<read_result_type>          read_handler_;
  ma::handler_storage<boost::system::error_code> stop_handler_;      

  frame_buffer_type         frame_buffer_;      
  boost::system::error_code read_error_;
  boost::system::error_code stop_error_;
  std::ptrdiff_t            read_handler_base_shift_;
      
  bool port_write_in_progress_;
  bool port_read_in_progress_;
  external_state_type external_state_;

  boost::asio::streambuf read_buffer_;
  std::string frame_head_;
  std::string frame_tail_;

  in_place_handler_allocator<256> write_allocator_;
  in_place_handler_allocator<256> read_allocator_;
}; // class cyclic_read_session 

} // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_HPP
