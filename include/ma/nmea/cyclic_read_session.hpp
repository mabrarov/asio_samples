//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
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
#include <algorithm>
#include <utility>
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/nmea/frame.hpp>
#include <ma/nmea/cyclic_read_session_fwd.hpp>
#include <ma/nmea/error.hpp>

namespace ma
{
  namespace nmea
  {
    class cyclic_read_session;
    typedef boost::shared_ptr<cyclic_read_session> cyclic_read_session_ptr;

    class cyclic_read_session 
      : private boost::noncopyable
      , public boost::enable_shared_from_this<cyclic_read_session>
    {
    private:
      typedef cyclic_read_session this_type;      
      BOOST_STATIC_CONSTANT(std::size_t, max_message_size = 512);           

    public:
      BOOST_STATIC_CONSTANT(std::size_t, min_read_buffer_size = max_message_size);
      BOOST_STATIC_CONSTANT(std::size_t, min_message_queue_size = 1);

      explicit cyclic_read_session(boost::asio::io_service& io_service,
        const std::size_t read_buffer_size, const std::size_t frame_buffer_size,
        const std::string& frame_head, const std::string& frame_tail);

      ~cyclic_read_session()
      {          
      } // ~cyclic_read_session

      boost::asio::serial_port& serial_port()
      {
        return serial_port_;
      } // serial_port

      void resest();

      template <typename Handler>
      void async_start(Handler handler)
      {        
        strand_.post(make_context_alloc_handler2(handler,
          boost::bind(&this_type::do_start<Handler>, shared_from_this(), _1)));
      }

      template <typename Handler>
      void async_stop(Handler handler)
      {
        strand_.post(make_context_alloc_handler2(handler,
          boost::bind(&this_type::do_stop<Handler>, shared_from_this(), _1)));
      }           

      // Handler::operator ()(const boost::system::error_code&, std::size_t)
      template <typename Handler, typename Iterator>
      void async_read_some(Iterator begin, Iterator end, Handler handler)
      {                
        strand_.post(make_context_alloc_handler2(handler,
          boost::bind(&this_type::do_read_some<Handler, Iterator>, shared_from_this(), begin, end, _1)));
      }

      template <typename ConstBufferSequence, typename Handler>
      void async_write(const ConstBufferSequence& buffer, Handler handler)
      {                
        strand_.post(make_context_alloc_handler2(handler, 
          boost::bind(&this_type::do_write<ConstBufferSequence, Handler>, shared_from_this(), buffer, _1)));
      }
    
    private:
      enum state_type
      {
        ready_to_start,
        start_in_progress,
        started,
        stop_in_progress,
        stopped
      };

      typedef boost::circular_buffer<frame_ptr> frame_buffer_type;

      template <typename Iterator>
      static std::size_t copy_buffer(const frame_buffer_type& buffer, Iterator begin, Iterator end, boost::system::error_code& error)
      {        
        std::size_t copy_size = std::min<std::size_t>(std::distance(begin, end), buffer.size());
        typedef frame_buffer_type::const_iterator buffer_iterator;
        buffer_iterator src_begin = buffer.begin();
        buffer_iterator src_end = boost::next(src_begin, copy_size);
        std::copy(src_begin, src_end, begin);
        error = boost::system::error_code();
        return copy_size;
      }

      class read_handler_base
      {
      private:
        typedef read_handler_base this_type;
        this_type& operator=(const this_type&);

      public:
        typedef std::size_t (*copy_func_type)(read_handler_base*, 
          const frame_buffer_type&, boost::system::error_code&);

        explicit read_handler_base(copy_func_type copy_func)
          : copy_func_(copy_func)          
        {
        } // read_handler_base

        std::size_t copy(const frame_buffer_type& buffer, boost::system::error_code& error)
        {
          return copy_func_(this, buffer, error);
        } // copy
        
      private:
        copy_func_type copy_func_;        
      }; // read_handler_base

      typedef boost::tuple<boost::system::error_code, std::size_t> read_result_type;

      template <typename Handler, typename Iterator>
      class wrapped_read_handler : public read_handler_base
      {
      private:
        typedef wrapped_read_handler<Handler, Iterator> this_type;
        this_type& operator=(const this_type&);

      public:        
        explicit wrapped_read_handler(Handler handler, Iterator begin, Iterator end)
          : read_handler_base(&this_type::do_copy)
          , handler_(handler)
          , begin_(begin)
          , end_(end)
        {
        } // wrapped_read_handler

        static std::size_t do_copy(read_handler_base* base, 
          const frame_buffer_type& buffer, boost::system::error_code& error)
        {
          this_type* this_ptr = static_cast<this_type*>(base);          
          return copy_buffer(buffer, this_ptr->begin_, this_ptr->end_, error);         
        } // do_copy

        friend void* asio_handler_allocate(std::size_t size, this_type* context)
        {
          return ma_asio_handler_alloc_helpers::allocate(size, context->handler_);
        } // asio_handler_allocate

        friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
        {
          ma_asio_handler_alloc_helpers::deallocate(pointer, size, context->handler_);
        }  // asio_handler_deallocate

        template <typename Function>
        friend void asio_handler_invoke(Function function, this_type* context)
        {
          ma_asio_handler_invoke_helpers::invoke(function, context->handler_);
        } // asio_handler_invoke

        void operator()(const read_result_type& result)
        {
          handler_(result.get<0>(), result.get<1>());
        } // operator()

      private:
        Handler handler_;
        Iterator begin_;
        Iterator end_;
      }; // wrapped_read_handler             

      template <typename Handler>
      void do_start(const Handler& handler)
      {
        boost::system::error_code error = start();
        io_service_.post(detail::bind_handler(handler, error));
      } // do_start

      template <typename Handler>
      void do_stop(const Handler& handler)
      { 
        if (boost::optional<boost::system::error_code> result = stop())        
        {          
          io_service_.post(detail::bind_handler(handler, *result));          
        }         
        else 
        {          
          stop_handler_.put(handler);
        }        
      } // do_stop

      template <typename Handler, typename Iterator>
      void do_read_some(const Iterator& begin, const Iterator& end, const Handler& handler)
      {
        if (boost::optional<boost::system::error_code> result = read_some())
        { 
          // Complete read operation "in place"
          if (*result)
          {            
            io_service_.post(detail::bind_handler(handler, *result, 0));
            return;
          }
          // Try to copy buffer data
          std::size_t frames_transferred = copy_buffer(frame_buffer_, begin, end, *result);
          frame_buffer_.erase_begin(frames_transferred);
          // Post the handler
          io_service_.post(detail::bind_handler(handler, *result, frames_transferred));
          return;
        }        
        put_read_handler(begin, end, handler);
      } // do_read_some

      template <typename Handler, typename Iterator>
      void put_read_handler(const Iterator& begin, const Iterator& end, const Handler& handler)
      {
        typedef wrapped_read_handler<Handler, Iterator> wrapped_handler_type;
        wrapped_handler_type wrapped_handler(handler, begin, end);
        wrapped_handler_type* wrapped_handler_ptr = boost::addressof(wrapped_handler);
        read_handler_base* base_handler_ptr = static_cast<read_handler_base*>(wrapped_handler_ptr);
        read_handler_base_shift_ = reinterpret_cast<char*>(base_handler_ptr) - reinterpret_cast<char*>(wrapped_handler_ptr);
        read_handler_.put(wrapped_handler);
      } // put_read_handler

      read_handler_base* get_read_handler() const
      {        
         return reinterpret_cast<read_handler_base*>(
           reinterpret_cast<char*>(read_handler_.target()) + read_handler_base_shift_);        
      } // get_read_handler

      template <typename ConstBufferSequence, typename Handler>
      void do_write(const ConstBufferSequence& buffer, const Handler& handler)
      {  
        if (boost::optional<boost::system::error_code> result = write(buffer, handler))        
        {          
          io_service_.post(detail::bind_handler(handler, *result));          
        }
      } // do_write

      boost::system::error_code start();
      boost::optional<boost::system::error_code> stop();
      boost::optional<boost::system::error_code> read_some();
      bool may_complete_stop() const;
      void complete_stop();

      template <typename ConstBufferSequence, typename Handler>
      boost::optional<boost::system::error_code> write(const ConstBufferSequence& buffer, const Handler& handler)
      {
        if (started != state_ || port_write_in_progress_)
        {                             
          return session_error::invalid_state;
        }
        boost::asio::async_write(serial_port_, buffer, strand_.wrap(
          make_custom_alloc_handler(write_allocator_, 
            boost::bind(&this_type::handle_write<Handler>, shared_from_this(), 
              boost::asio::placeholders::error, boost::make_tuple<Handler>(handler)))));
        port_write_in_progress_ = true;        
        return boost::optional<boost::system::error_code>();
      }
     
      template <typename Handler>
      void handle_write(const boost::system::error_code& error, boost::tuple<Handler> handler)
      {         
        port_write_in_progress_ = false;
        io_service_.post(detail::bind_handler(handler.get<0>(), error));
        if (stop_in_progress == state_ && !port_read_in_progress_)
        {
          state_ = stopped;
          post_stop_handler();
        }
      } 

      void read_until_head();
      void read_until_tail();
      void handle_read_head(const boost::system::error_code& error, const std::size_t bytes_transferred);
      void handle_read_tail(const boost::system::error_code& error, const std::size_t bytes_transferred);      
      void post_stop_handler();
      
      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;
      boost::asio::serial_port serial_port_;      
      ma::handler_storage<read_result_type> read_handler_;
      ma::handler_storage<boost::system::error_code> stop_handler_;      
      frame_buffer_type frame_buffer_;      
      boost::system::error_code read_error_;
      boost::system::error_code stop_error_;
      std::ptrdiff_t read_handler_base_shift_;
      state_type state_;
      bool port_write_in_progress_;
      bool port_read_in_progress_;
      boost::asio::streambuf read_buffer_;
      std::string frame_head_;
      std::string frame_tail_;
      in_place_handler_allocator<256> write_allocator_;
      in_place_handler_allocator<256> read_allocator_;
    }; // class cyclic_read_session 

  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_HPP