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
#include <ma/handler_storage.hpp>

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
      typedef boost::tuple<message_ptr, boost::system::error_code> read_arg_type;            

      BOOST_STATIC_CONSTANT(size_type, max_message_size = 512);           

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
        : frame_head_(frame_head)
        , frame_tail_(frame_tail)                  
        , io_service_(io_service)
        , strand_(io_service)
        , stream_(io_service)
        , read_handler_(io_service)        
        , stop_handler_(io_service)
        , started_(false)
        , stopped_(false)        
        , write_in_progress_(false)
        , read_in_progress_(false)
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

      boost::asio::io_service& io_service()
      {
        return io_service_;
      }

      boost::asio::io_service& get_io_service()
      {
        return io_service_;
      }

      void resest()
      {
        read_buf_.clear();        
        read_error_ = boost::system::error_code();
        read_buf_overflow_ = false;

        stream_read_buf_.consume(
          boost::asio::buffer_size(stream_read_buf_.data()));        

        started_ = false;
        stopped_ = false;
      }

      template <typename Handler>
      void async_start(Handler handler)
      {        
        strand_.dispatch
        (
          make_context_alloc_handler
          (
            handler,
            boost::bind
            (
              &this_type::do_start<Handler>, 
              shared_from_this(),
              boost::make_tuple(handler)
            )
          )
        );
      }

      template <typename Handler>
      void async_stop(Handler handler)
      {
        strand_.dispatch
        (
          make_context_alloc_handler
          ( 
            handler,
            boost::bind
            (
              &this_type::do_stop<Handler>, 
              shared_from_this(),
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
              &this_type::do_write<Handler>, 
              shared_from_this(),
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
              &this_type::do_read<Handler>, 
              shared_from_this(),
              boost::ref(message), 
              boost::make_tuple(handler)              
            )
          )
        );
      }        
    
    private:        
      template <typename Handler>
      void do_start(boost::tuple<Handler> handler)
      {
        if (stopped_ || stop_handler_.has_target())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (started_)
        {          
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
          // Start handshake: start internal activity.
          if (!read_in_progress_)
          {
            read_until_head();
          }

          // Complete handshake immediately
          started_ = true;

          // Signal successful handshake completion.
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::system::error_code()
            )
          );
        }
      } // do_start

      template <typename Handler>
      void do_stop(boost::tuple<Handler> handler)
      { 
        if (stopped_)
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (stop_handler_.has_target())
        {          
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
          // Start shutdown

          // Do shutdown - abort inner operations
          stream_.close(stop_error_);
          
          // Check for shutdown continuation
          if (read_handler_.has_target()
            || write_in_progress_ 
            || read_in_progress_)
          {
            read_handler_.cancel();
            // Wait for others operations' completion
            stop_handler_.store(
              boost::asio::error::operation_aborted,
              boost::get<0>(handler));
          }        
          else
          {
            stopped_ = true;
            // Signal shutdown completion
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                stop_error_
              )
            );
          }
        }        
      } // do_stop
      
      template <typename Handler>
      void do_write(const message_ptr& message, boost::tuple<Handler> handler)
      {  
        if (stopped_ || stop_handler_.has_target())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (!started_ || write_in_progress_)
        {          
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
                  message,
                  handler
                )
              )
            )
          );
          write_in_progress_ = true;
        }
      } // do_write

      template <typename Handler>
      void handle_write(const boost::system::error_code& error,         
        const message_ptr&, boost::tuple<Handler> handler)
      {         
        write_in_progress_ = false;        
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            error
          )
        );
        if (stop_handler_.has_target() && !read_in_progress_)
        {
          stopped_ = true;
          // Signal shutdown completion
          stop_handler_.post(stop_error_);
        }
      } 

      template <typename Handler>
      void do_read(message_ptr& message, boost::tuple<Handler> handler)
      {
        if (stopped_ || stop_handler_.has_target())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        }
        else if (!started_ || read_handler_.has_target())
        {          
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
        }
        else if (!read_buf_.empty())
        {
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
        }
        else if (read_error_)
        {            
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
        }
        else
        {          
          // If can't immediately complete then start waiting for completion
          // Start message constructing
          if (!read_in_progress_)
          {
            read_until_head();
          }

          // Wait for the ready message
          read_handler_.store
          (
            read_arg_type
            (
              message_ptr(), 
              boost::asio::error::operation_aborted
            ),          
            make_context_alloc_handler
            (
              boost::get<0>(handler), 
              boost::bind
              (
                &this_type::handle_read<Handler>, 
                _1,
                boost::ref(message), 
                handler
              )
            )          
          );
        }        
      }      
      
      template <typename Handler>
      static void handle_read(const read_arg_type& arg, 
        message_ptr& target, boost::tuple<Handler> handler)
      {
        target = boost::get<0>(arg);
        boost::get<0>(handler)(boost::get<1>(arg));
      }

      void read_until_head()
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
        read_in_progress_ = true;          
      }

      void read_until_tail()
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
        read_in_progress_ = true;
      }      

      void handle_read_head(const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {         
        read_in_progress_ = false;        
        if (stop_handler_.has_target())
        {
          read_handler_.cancel();
          if (!write_in_progress_)
          {
            stopped_ = true;
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else if (!error)
        {
          // We do not need in-between-frame-garbage and frame's head
          stream_read_buf_.consume(bytes_transferred);                    
          read_until_tail();
        }
        else if (read_handler_.has_target())
        {
          read_handler_.post(read_arg_type(message_ptr(), error));
        }
        else
        {
          // Store error for the next outer read operation.
          read_error_ = error;          
        }
      }   

      void handle_read_tail(const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {                  
        read_in_progress_ = false;        
        if (stop_handler_.has_target())
        {
          read_handler_.cancel();
          if (!write_in_progress_)
          {
            stopped_ = true;
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else if (!error)        
        {                   
          typedef boost::asio::streambuf::const_buffers_type const_buffers_type;
          typedef boost::asio::buffers_iterator<const_buffers_type> buffers_iterator;
          const_buffers_type committed_buffers(stream_read_buf_.data());
          buffers_iterator data_begin(buffers_iterator::begin(committed_buffers));
          buffers_iterator data_end(data_begin + bytes_transferred - frame_tail_.length());
          message_ptr new_message(new message_type(data_begin, data_end));
          // Consume processed data
          stream_read_buf_.consume(bytes_transferred);
          // Continue inner operations loop.
          read_until_head();
          // If there is waiting read operation - complete it            
          if (read_handler_.has_target())
          {              
            read_handler_.post(read_arg_type(new_message, error));
          } 
          else
          {
            // else push ready message into the cyclic read buffer
            if (read_buf_.full())
            {                
              read_buf_overflow_ = true;                
            }
            read_buf_.push_back(new_message);              
          }          
        }
        else if (read_handler_.has_target())
        {
          read_handler_.post(read_arg_type(message_ptr(), error));
        }
        else
        {
          // Store error for the next outer read operation.
          read_error_ = error;          
        }
      }
                
      std::string frame_head_;
      std::string frame_tail_;                    
      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;
      next_layer_type stream_;               
      ma::handler_storage<read_arg_type> read_handler_;      
      ma::handler_storage<boost::system::error_code> stop_handler_;      
      bool started_;
      bool stopped_;      
      bool write_in_progress_;
      bool read_in_progress_;
      boost::asio::streambuf stream_read_buf_;        
      handler_allocator stream_write_allocator_;
      handler_allocator stream_read_allocator_;
      boost::system::error_code read_error_;
      boost::system::error_code stop_error_;
      read_buf_type read_buf_;
      bool read_buf_overflow_;
    }; // class cyclic_read_session 

  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READ_SESSION_HPP