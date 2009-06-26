//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_DETAIL_CYCLIC_READING_SESSION_HPP
#define MA_NMEA_DETAIL_CYCLIC_READING_SESSION_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <string>
#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/detail/atomic_count.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/work_handler.hpp>
#include <ma/handler_storage.hpp>

namespace ma
{
  namespace nmea
  {  
    namespace detail
    {  
      template <typename AsyncStream>
      class cyclic_reading_session : private boost::noncopyable      
      {
      public:
        typedef std::size_t size_type;
        typedef boost::shared_ptr<std::string> message_type;

      private:
        typedef handler_storage<boost::system::error_code> handler_storage_type;
        typedef cyclic_reading_session<AsyncStream> this_type;      
        typedef boost::circular_buffer<message_type> read_message_buffer_type;
        BOOST_STATIC_CONSTANT(size_type, max_message_size = 512);
        BOOST_STATIC_CONSTANT(size_type, read_continue_threhold = 64);      

      public:      
        typedef AsyncStream next_layer_type;
        typedef typename next_layer_type::lowest_layer_type lowest_layer_type;
        typedef boost::intrusive_ptr<this_type> pointer;     
        typedef typename read_message_buffer_type::capacity_type read_capacity_type;
        BOOST_STATIC_CONSTANT(size_type, min_read_buffer_size = max_message_size);
        BOOST_STATIC_CONSTANT(read_capacity_type, min_read_message_buffer_capacity = 1);        

        this_type* prev_;
        pointer next_;    
        handler_allocator service_handler_allocator_;

        explicit cyclic_reading_session(
          boost::asio::io_service& io_service,
          const size_type read_buffer_size,
          const read_capacity_type read_message_buffer_capacity,
          const std::string& frame_head,
          const std::string& frame_tail)
          : prev_(0)
          , next_(0)          
          , frame_head_(frame_head)
          , frame_tail_(frame_tail)
          , ref_count_(0)
          , io_service_(io_service)
          , strand_(io_service)
          , stream_(io_service)                    
          , handshake_done_(false)
          , shutdown_done_(false)
          , writing_(false)
          , reading_(false)
          , read_buffer_(read_buffer_size)                    
          , read_target_(0)          
          , read_message_buffer_(read_message_buffer_capacity)
          , message_buffer_overflow_(false)
        {
          if (max_message_size > read_buffer_size)
          {
            boost::throw_exception(std::runtime_error("too small read_buffer_size"));
          }
          if (frame_head.length() > max_message_size)
          {
            boost::throw_exception(std::runtime_error("too large frame_head"));
          }
          if (frame_tail.length() > max_message_size)
          {
            boost::throw_exception(std::runtime_error("too large frame_tail"));
          }
        }

        ~cyclic_reading_session()
        {          
        }        

        next_layer_type& next_layer()
        {
          return stream_;
        }

        lowest_layer_type& lowest_layer()
        {
          return stream_.lowest_layer();
        }

        void cancel_for_destroy()
        {
          // Close all user operations.          
          if (read_handler_)
          {
            read_handler_(boost::asio::error::operation_aborted);
          }
          if (write_handler_)
          {
            write_handler_(boost::asio::error::operation_aborted);
          }
          if (shutdown_handler_)
          {
            shutdown_handler_(boost::asio::error::operation_aborted);
          }
        }               

        template <typename Handler>
        void async_handshake(Handler handler)
        {        
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_handshake<Handler>, pointer(this), boost::make_tuple(handler))));
        }

        template <typename Handler>
        void async_shutdown(Handler handler)
        {
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_shutdown<Handler>, pointer(this), boost::make_tuple(handler))));
        }
        
        template <typename Handler>
        void async_write(const message_type& message, Handler handler)
        {        
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_write<Handler>, pointer(this), message, boost::make_tuple(handler))));
        }

        template <typename Handler>
        void async_read(message_type& message, Handler handler)
        {        
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_read<Handler>, pointer(this), boost::ref(message), boost::make_tuple(handler))));
        }        
      
      private:
        friend void intrusive_ptr_add_ref(this_type* ptr)
        {
          ++ptr->ref_count_;
        }

        friend void intrusive_ptr_release(this_type* ptr)
        {
          if (0 == --ptr->ref_count_)
          {
            delete ptr;
          }
        }

        template <typename Handler>
        void start_handshake(boost::tuple<Handler> handler)
        {                  
          if (shutdown_done_)
          {          
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
          }          
          else if (handshake_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_connected));
          }
          else
          {          
            if (!reading_)
            {
              start_read_until_head();          
            }
            handshake_done_ = true;            
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::system::error_code()));              
          }
        }

        template <typename Handler>
        void start_shutdown(boost::tuple<Handler> handler)
        {        
          if (shutdown_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
          }
          else if (shutdown_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_started));
          }
          else if (!handshake_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::not_connected));
          }          
          else 
          {            
            shutdown_done_ = true;            
            if (read_handler_)
            {
              read_handler_(boost::asio::error::operation_aborted);
            }
            if (write_handler_)
            {
              write_handler_(boost::asio::error::operation_aborted);
            }
            stream_.close(shutdown_error_);
            if (!reading_ && !writing_)
            {
              io_service_.post(boost::asio::detail::bind_handler(
                boost::get<0>(handler), shutdown_error_));
            }
            else
            {              
              handler_storage_type new_shutdown_handler(
                make_work_handler(io_service_, boost::get<0>(handler)));
              shutdown_handler_.swap(new_shutdown_handler);
            }            
          }          
        }
        
        template <typename Handler>
        void start_write(const message_type& message, boost::tuple<Handler> handler)
        {          
          if (shutdown_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
          }
          else if (!handshake_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::not_connected));
          }
          else if (write_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_started));
          }          
          else if (!writing_)
          {
            handler_storage_type new_write_handler(
              make_work_handler(io_service_, boost::get<0>(handler)));
            write_handler_.swap(new_write_handler);
            write_message_buffer_ = message;
            start_write_all();
          }
          else
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_not_supported));
          }
        }

        template <typename Handler>
        void start_read(message_type& message, boost::tuple<Handler> handler)
        {          
          if (shutdown_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
          }
          else if (!handshake_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::not_connected));
          }
          else if (read_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_started));
          }          
          else if (!read_message_buffer_.empty())
          {
            message = read_message_buffer_.front();
            read_message_buffer_.pop_front();
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::system::error_code()));
          }
          else if (last_read_error_)
          {          
            boost::system::error_code error(last_read_error_);
            last_read_error_ = boost::system::error_code();
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), error));
          }
          else
          {
            read_target_ = &message;
            handler_storage_type new_read_handler(
              make_work_handler(io_service_, boost::get<0>(handler)));
            read_handler_.swap(new_read_handler);              
            if (!reading_)
            {
              start_read_until_head();
            }
          }
        }        

        void start_write_all()
        {                                 
          boost::asio::async_write(stream_, boost::asio::buffer(*write_message_buffer_), 
            strand_.wrap(make_custom_alloc_handler(write_handler_allocator_, 
              boost::bind(&this_type::handle_write, pointer(this), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
          writing_ = true;
        }

        void start_read_until_head()
        {                                 
          boost::asio::async_read_until(stream_, read_buffer_, frame_head_,
            strand_.wrap(make_custom_alloc_handler(read_handler_allocator_, 
              boost::bind(&this_type::handle_read_head, pointer(this), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
          reading_ = true;
        }

        void start_read_until_tail()
        {          
          boost::asio::async_read_until(stream_, read_buffer_, frame_tail_,
            strand_.wrap(make_custom_alloc_handler(read_handler_allocator_, 
              boost::bind(&this_type::handle_read_tail, pointer(this), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
          reading_ = true;        
        }

        void handle_write(const boost::system::error_code& error, const std::size_t bytes_transferred)
        {          
          writing_ = false;
          if (shutdown_done_)
          {
            if (shutdown_handler_ && !writing_ && !reading_)
            {
              shutdown_handler_(shutdown_error_);
            }
          }
          else
          {
            // Exclude written data from write message buffer
            write_message_buffer_.reset();
            if (write_handler_)
            {
              // Write in progress - call write handler
              write_handler_(error);
            }
            if (write_message_buffer_)
            {
              // If write message buffer not empty - start internal write operation
              start_write_all();
            }
          }
        }   

        void handle_read_head(const boost::system::error_code& error, const std::size_t bytes_transferred)
        {          
          reading_ = false;
          if (shutdown_done_)
          {
            if (shutdown_handler_ && !writing_ && !reading_)
            {
              shutdown_handler_(shutdown_error_);
            }
          }
          else  if (error)
          {
            if (read_handler_)
            {
              read_handler_(error);
            }
            else 
            {
              last_read_error_ = error;
            }
          }
          else
          {
            // We do not need in-between-frame-garbage and frame's head
            read_buffer_.consume(bytes_transferred);
            start_read_until_tail();
          }          
        }   

        void handle_read_tail(const boost::system::error_code& error, const std::size_t bytes_transferred)
        {          
          reading_ = false;
          if (shutdown_done_)
          {
            if (shutdown_handler_ && !writing_ && !reading_)
            {
              shutdown_handler_(shutdown_error_);
            }
          }
          else if (error)
          {
            if (read_handler_)
            {
              read_handler_(error);
            }
            else 
            {
              last_read_error_ = error;
            }
          }
          else
          {
            // Parse committed bytes
            typedef boost::asio::streambuf::const_buffers_type const_buffers_type;
            typedef boost::asio::buffers_iterator<const_buffers_type> buffers_iterator;
            const_buffers_type committed_buffers(read_buffer_.data());
            buffers_iterator data_begin(buffers_iterator::begin(committed_buffers));
            buffers_iterator data_end(data_begin + bytes_transferred - frame_tail_.length());
            message_type parsed_message(new std::string(data_begin, data_end));              
            // Consume processed data
            read_buffer_.consume(bytes_transferred);            
            // Start reading next frame
            start_read_until_head();                        
            // If has awating handler than call it
            // else push ready message into the cyclic message buffer
            if (read_handler_)
            {              
              *read_target_ = parsed_message;
              read_handler_(error);
            } 
            else
            {
              if (read_message_buffer_.full())
              {
                //todo
                message_buffer_overflow_ = true;                
              }
              read_message_buffer_.push_back(parsed_message);              
            }            
          }          
        }
            
        std::string frame_head_;
        std::string frame_tail_;      
        boost::detail::atomic_count ref_count_;      
        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;
        next_layer_type stream_;               
        handler_storage_type shutdown_handler_;
        handler_storage_type read_handler_;
        handler_storage_type write_handler_;
        bool handshake_done_;
        bool shutdown_done_;
        bool writing_;
        bool reading_;
        boost::asio::streambuf read_buffer_;        
        handler_allocator write_handler_allocator_;
        handler_allocator read_handler_allocator_;
        boost::system::error_code last_read_error_;      
        boost::system::error_code shutdown_error_;      
        message_type* read_target_;
        message_type write_message_buffer_;
        read_message_buffer_type read_message_buffer_;
        bool message_buffer_overflow_;
      }; // class cyclic_reading_session

    } // namespace detail
  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_DETAIL_CYCLIC_READING_SESSION_HPP