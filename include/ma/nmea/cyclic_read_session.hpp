//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READ_SESSION_HPP
#define MA_NMEA_CYCLIC_READ_SESSION_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <string>
#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/condition.hpp>

namespace ma
{
  namespace nmea
  {      
    template <typename AsyncStream>
    class cyclic_read_session 
      : private boost::noncopyable
      , public boost::enable_shared_from_this<cyclic_read_session<AsyncStream> >
    {
    public:
      typedef std::size_t size_type;
      typedef std::string message_type;
      typedef boost::shared_ptr<message_type> message_ptr;

    private:
      typedef cyclic_read_session<AsyncStream> this_type;      
      typedef boost::circular_buffer<message_ptr> read_buf_type;      
      struct cancel_tag : private boost::noncopyable {};
      typedef boost::shared_ptr<cancel_tag> cancel_token;
      typedef boost::weak_ptr<cancel_tag> cancel_monitor;
      typedef boost::tuple<message_ptr, boost::system::error_code> read_argument_type;
      typedef boost::system::error_code write_argument_type;
      typedef boost::system::error_code shutdown_argument_type;      

      BOOST_STATIC_CONSTANT(size_type, max_message_size = 512);
      BOOST_STATIC_CONSTANT(size_type, read_continue_threhold = 64);
      

    public:
      typedef boost::shared_ptr<this_type> pointer;
      typedef AsyncStream next_layer_type;
      typedef typename next_layer_type::lowest_layer_type lowest_layer_type;      
      typedef typename read_buf_type::capacity_type read_capacity_type;
      BOOST_STATIC_CONSTANT(size_type, min_stream_read_buf_size = max_message_size);
      BOOST_STATIC_CONSTANT(read_capacity_type, min_read_buf_capacity = 1);

      explicit cyclic_read_session(
        boost::asio::io_service& io_service,
        const size_type stream_read_buf_size,
        const read_capacity_type read_buf_capacity,
        const std::string& frame_head,
        const std::string& frame_tail)
        : cancel_token_(new cancel_tag())
        , frame_head_(frame_head)
        , frame_tail_(frame_tail)                  
        , io_service_(io_service)
        , strand_(io_service)
        , stream_(io_service)
        , read_condition_(io_service)
        , write_condition_(io_service)
        , shutdown_condition_(io_service)
        , handshake_done_(false)
        , read_in_progress_(false)
        , write_in_progress_(false)
        , shutdown_in_progress_(false)
        , stream_write_in_progress_(false)
        , stream_read_in_progress_(false)
        , stream_read_buf_(stream_read_buf_size)
        , read_buf_(read_buf_capacity)
        , read_buf_overflow_(false)
      {
        if (max_message_size > stream_read_buf_size)
        {
          boost::throw_exception(std::runtime_error("too small stream_read_buf_size"));
        }
        if (frame_head.length() > stream_read_buf_size)
        {
          boost::throw_exception(std::runtime_error("too large frame_head"));
        }
        if (frame_tail.length() > stream_read_buf_size)
        {
          boost::throw_exception(std::runtime_error("too large frame_tail"));
        }
      }

      ~cyclic_read_session()
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

      template <typename Handler>
      void async_handshake(Handler handler)
      {        
        strand_.dispatch
        (
          make_context_alloc_handler
          (
            handler,
            boost::bind
            (
              &this_type::start_handshake<Handler>, 
              shared_from_this(), 
              cancel_monitor(cancel_token_),
              boost::make_tuple(handler)
            )
          )
        );
      }

      template <typename Handler>
      void async_shutdown(Handler handler)
      {
        // Cancel all pending operations
        cancel_token_.reset(new cancel_tag());

        strand_.dispatch
        (
          make_context_alloc_handler
          ( 
            handler,
            boost::bind
            (
              &this_type::start_shutdown<Handler>, 
              shared_from_this(), 
              cancel_monitor(cancel_token_),
              boost::make_tuple(handler)
            )
          )
        );
      }
      
      template <typename Handler>
      void async_write(const message_ptr& message, Handler handler)
      {                
        strand_.dispatch
        (
          make_context_alloc_handler
          (
            handler,
            boost::bind
            (
              &this_type::start_write<Handler>, 
              shared_from_this(), 
              cancel_monitor(cancel_token_),
              message, 
              boost::make_tuple(handler)              
            )
          )
        );
      }

      template <typename Handler>
      void async_read(message_ptr& message, Handler handler)
      {                
        strand_.dispatch
        (
          make_context_alloc_handler
          (
            handler,
            boost::bind
            (
              &this_type::start_read<Handler>, 
              shared_from_this(), 
              cancel_monitor(cancel_token_),
              boost::ref(message), 
              boost::make_tuple(handler)              
            )
          )
        );
      }        
    
    private:        
      template <typename Handler>
      void start_handshake(cancel_monitor op_monitor, boost::tuple<Handler> handler)
      {  
        // Check operation cancellation.
        if (op_monitor.expired())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );
          return;
        }
        // Check outer state.
        if (handshake_done_ 
          || read_in_progress_ 
          || write_in_progress_ 
          || shutdown_in_progress_)
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
          return;
        }

        // Start handshake: start internal activity.
        if (!stream_read_in_progress_)
        {
          start_read_until_head();
        }

        // Complete handshake immidiately
        handshake_done_ = true;

        // Signal succesfull handshake completion.
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            boost::system::error_code()
          )
        );        
      } // start_handshake

      template <typename Handler>
      void start_shutdown(cancel_monitor op_monitor, boost::tuple<Handler> handler)
      { 
        // Check operation cancellation.
        if (op_monitor.expired())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );
          return;
        }
        // Check outer state.
        if (!handshake_done_ || shutdown_in_progress_)
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
          return;
        }

        // Start shutdown
        shutdown_in_progress_ = true;

        // Do shutdown: abort outer operations
        if (read_in_progress_)
        {          
          complete_read();
          read_condition_.cancel();
        }
        if (write_in_progress_)
        {   
          complete_write();
          write_condition_.cancel();
        }

        // Do shutdown: abort inner operations
        stream_.close(shutdown_error_);        

        // Check for immidiate completion
        if (read_in_progress_ 
          || write_in_progress_ 
          || stream_write_in_progress_ 
          || stream_read_in_progress_)
        {    
          // If can't immidiately complete then start waiting for completion
          shutdown_condition_.async_wait
          (
            boost::asio::error::operation_aborted,
            boost::get<0>(handler)
          );
          return;
        }

        // Complete shutdown immidiately
        complete_shutdown();

        // Signal succesfull shutdown completion.
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            shutdown_error_
          )
        );
      } // start_shutdown
      
      void complete_shutdown()
      {
        shutdown_in_progress_ = false;
        handshake_done_ = false;
        
        read_buf_.clear();        
        read_error_ = boost::system::error_code();
        read_buf_overflow_ = false;

        stream_read_buf_.consume(
          boost::asio::buffer_size(stream_read_buf_.data()));        
      }

      void complete_waiting_shutdown()
      {
        // Check for shutdown completion
        if (shutdown_in_progress_
          && !read_in_progress_ 
          && !write_in_progress_ 
          && !stream_write_in_progress_ 
          && !stream_read_in_progress_)
        {              
          // Complete shutdown immidiately
          complete_shutdown();

          // Signal succesfull shutdown completion.
          shutdown_condition_.fire_now(shutdown_error_);
        }
      }
      
      template <typename Handler>
      void start_write(cancel_monitor op_monitor, 
        const message_ptr& message, boost::tuple<Handler> handler)
      {  
        // Check operation cancellation.
        if (op_monitor.expired())
        {
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );
          return;
        }
        // Check outer state.
        if (!handshake_done_ 
          || write_in_progress_ 
          || shutdown_in_progress_)
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
          return;
        }

        if (stream_write_in_progress_)
        {
          // Never here
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
        }
        else
        {
          start_write_message(message, boost::get<0>(handler));
        }

        // Wait for the message write completion
        write_condition_.async_wait
        (          
          boost::asio::error::operation_aborted,
          boost::get<0>(handler)
        );
      }

      void complete_write()
      {
        write_in_progress_ = false;
      }

      void complete_waiting_write(const boost::system::error_code& error)
      {        
        complete_write();
        write_condition_.fire_now(error);
      }      

      template <typename Handler>
      void start_write_message(const message_ptr& message, Handler handler)
      {                                 
        boost::asio::async_write
        (
          stream_, 
          boost::asio::buffer(*message), 
          strand_.wrap
          (
            make_custom_alloc_handler
            (
              stream_write_allocator_, 
              boost::bind
              (
                &this_type::handle_write<handler>, 
                shared_from_this(),                
                boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred,
                message,
                boost::make_tuple(handler)
              )
            )
          )
        );

        stream_write_in_progress_ = true;
      }

      template <typename Handler>
      void handle_write(const boost::system::error_code& error, 
        const std::size_t bytes_transferred,
        const message_ptr&, boost::tuple<Handler>)
      {         
        stream_write_in_progress_ = false;
        
        // Check for write completion
        if (write_in_progres_)
        {
          complete_waiting_write(error);         
        }

        // Check for shutdown completion
        complete_waiting_shutdown();
      } 

      template <typename Handler>
      void start_read(cancel_monitor op_monitor, 
        message_ptr& message, boost::tuple<Handler> handler)
      { 
        // Check operation cancellation.
        if (op_monitor.expired())
        {
          message = message_ptr();
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );
          return;
        }
        // Check outer state.
        if (!handshake_done_ 
          || read_in_progress_ 
          || shutdown_in_progress_)
        {          
          message = message_ptr();
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
          return;
        }      

        // Start read operation
        read_in_progress_ = true;

        // Check for immidiate completion: check for ready data        
        if (!read_buf_.empty())
        {
          complete_read();

          message = read_buf_.front();
          read_buf_.pop_front();
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::system::error_code()
            )
          );          
          return;
        }

        // Check for immidiate completion: check for read error
        if (read_error_)
        {
          complete_read();

          message = message_ptr();
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              read_error_
            )
          );
          read_error_= boost::system::error_code();
          return;
        }

        // If can't immidiately complete then start waiting for completion
        // Start message constructing
        if (!stream_read_in_progress_)
        {
          start_read_until_head();
        }

        // Wait for the ready message
        read_condition_.async_wait
        (
          read_argument_type
          (
            message_ptr(),
            boost::asio::error::operation_aborted
          ),
          make_context_alloc_handler
          (
            boost::get<0>(handler),
            boost::bind
            (
              &this_type::read_handler_adaptor<Handler>, 
              _1,
              boost::ref(message), 
              handler
            )
          )
        );        
      }

      void complete_read()
      {
        read_in_progress_ = false;
      }

      void complete_waiting_read(const message_ptr& message_ptr, 
        const boost::system::error_code& error)
      {        
        complete_read();
        read_condition_.fire_now(read_argument_type(message_ptr, error));
      }

      void complete_waiting_read(const boost::system::error_code& error)
      {        
        complete_waiting_read(message_ptr(), error);
      }

      template <typename Handler>
      static void read_handler_adaptor(const read_argument_type& arg, 
        message_ptr& target, boost::tuple<Handler> handler)
      {
        target = boost::get<0>(arg);
        boost::get<0>(handler)(boost::get<1>(arg));
      }

      void start_read_until_head()
      {                                 
        boost::asio::async_read_until
        (
          stream_, stream_read_buf_, frame_head_,
          strand_.wrap
          (
            make_custom_alloc_handler
            (
              stream_read_allocator_, 
              boost::bind
              (
                &this_type::handle_read_head, 
                shared_from_this(), 
                boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred
              )
            )
          )
        );          
        
        stream_read_in_progress_ = true;          
      }

      void start_read_until_tail()
      {                    
        boost::asio::async_read_until
        (
          stream_, stream_read_buf_, frame_tail_,
          strand_.wrap
          (
            make_custom_alloc_handler
            (
              stream_read_allocator_, 
              boost::bind
              (
                &this_type::handle_read_tail, 
                shared_from_this(), 
                boost::asio::placeholders::error, 
                boost::asio::placeholders::bytes_transferred
              )
            )
          )
        );          
                
        stream_read_in_progress_ = true;
      }      

      void handle_read_head(const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {         
        stream_read_in_progress_ = false;
        
        if (!error)
        {
          // We do not need in-between-frame-garbage and frame's head
          stream_read_buf_.consume(bytes_transferred);          
          if (!shutdown_in_progress_)
          {            
            // Continue inner operations loop.
            start_read_until_tail();
          }
        }
        else if (read_in_progress_)
        {
          complete_waiting_read(error);
        }
        else
        {
          // Store error for the next outer read operation.
          read_error_ = error;          
        }

        // Check for shutdown completion
        complete_waiting_shutdown();
      }   

      void handle_read_tail(const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {                  
        stream_read_in_progress_ = false;
        
        if (!error)
        {         
          if (!shutdown_in_progress_)
          {  
            typedef boost::asio::streambuf::const_buffers_type const_buffers_type;
            typedef boost::asio::buffers_iterator<const_buffers_type> buffers_iterator;
            const_buffers_type committed_buffers(stream_read_buf_.data());
            buffers_iterator data_begin(buffers_iterator::begin(committed_buffers));
            buffers_iterator data_end(data_begin + bytes_transferred - frame_tail_.length());
            message_ptr parsed_message(new message_type(data_begin, data_end));
            // Consume processed data
            stream_read_buf_.consume(bytes_transferred);
            // Continue inner operations loop.
            start_read_until_head();
            // If there is wating read operation - complete it            
            if (read_in_progress_)
            {              
              complete_waiting_read(parsed_message, error);
            } 
            else
            {
              // else push ready message into the cyclic read buffer
              if (read_buf_.full())
              {                
                read_buf_overflow_ = true;                
              }
              read_buf_.push_back(parsed_message);              
            }            
          }
        }
        else if (read_in_progress_)
        {
          complete_waiting_read(error);
        }
        else
        {
          // Store error for the next outer read operation.
          read_error_ = error;          
        }

        // Check for shutdown completion
        complete_waiting_shutdown();
      }
          
      cancel_token cancel_token_;
      std::string frame_head_;
      std::string frame_tail_;                    
      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;
      next_layer_type stream_;               
      ma::condition<read_argument_type> read_condition_;
      ma::condition<write_argument_type> write_condition_;
      ma::condition<shutdown_argument_type> shutdown_condition_;      
      bool handshake_done_;
      bool read_in_progress_;
      bool write_in_progress_;
      bool shutdown_in_progress_;
      bool stream_write_in_progress_;
      bool stream_read_in_progress_;
      boost::asio::streambuf stream_read_buf_;        
      handler_allocator stream_write_allocator_;
      handler_allocator stream_read_allocator_;
      boost::system::error_code read_error_;
      boost::system::error_code shutdown_error_;
      read_buf_type read_buf_;
      bool read_buf_overflow_;
    }; // class cyclic_read_session 

  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_HPP