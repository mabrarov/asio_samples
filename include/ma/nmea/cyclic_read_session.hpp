//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READ_SESSION_HPP
#define MA_NMEA_CYCLIC_READ_SESSION_HPP

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
#include <ma/bind_asio_handler.hpp>

namespace ma
{
  namespace nmea
  { 
    class cyclic_read_session;
    typedef std::string message_type;
    typedef boost::shared_ptr<message_type> message_ptr;    
    typedef boost::shared_ptr<cyclic_read_session> cyclic_read_session_ptr;

    class cyclic_read_session 
      : private boost::noncopyable
      , public boost::enable_shared_from_this<cyclic_read_session>
    {
    private:
      typedef cyclic_read_session this_type;
      enum state_type
      {
        ready_to_start,
        start_in_progress,
        started,
        stop_in_progress,
        stopped
      };      
      BOOST_STATIC_CONSTANT(std::size_t, max_message_size = 512);           

    public:
      BOOST_STATIC_CONSTANT(std::size_t, min_read_buffer_size = max_message_size);
      BOOST_STATIC_CONSTANT(std::size_t, min_message_queue_size = 1);

      explicit cyclic_read_session(boost::asio::io_service& io_service,
        const std::size_t read_buffer_size, const std::size_t message_queue_size,
        const std::string& frame_head, const std::string& frame_tail);
      ~cyclic_read_session();      

      boost::asio::serial_port& serial_port();      
      void resest();

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
      
      template <typename ConstBufferSequence, typename Handler>
      void async_write(const ConstBufferSequence& buffer, Handler handler)
      {                
        strand_.dispatch
        (
          make_context_alloc_handler
          (
            handler,
            boost::bind
            (
              &this_type::do_write<ConstBufferSequence, Handler>, 
              shared_from_this(),
              buffer, 
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
      void do_start(const boost::tuple<Handler>& handler)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (ready_to_start != state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );          
        }
        else
        {
          // Start handshake
          state_ = start_in_progress;

          // Start internal activity
          if (!port_read_in_progress_)
          {
            read_until_head();
          }

          // Handshake completed
          state_ = started;

          // Signal successful handshake completion.
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::system::error_code()
            )
          );
        }
      } // do_start

      template <typename Handler>
      void do_stop(const boost::tuple<Handler>& handler)
      { 
        if (stopped == state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (stop_in_progress == state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
        }
        else 
        {
          // Start shutdown
          state_ = stop_in_progress;

          // Do shutdown - abort inner operations
          serial_port_.close(stop_error_);
          
          // Check for shutdown continuation
          if (read_handler_.has_target()
            || port_write_in_progress_ 
            || port_read_in_progress_)
          {
            if (read_handler_.has_target())
            {
              read_handler_.post(boost::asio::error::operation_aborted);
            }            
            // Wait for others operations' completion
            stop_handler_.store(boost::get<0>(handler));
          }        
          else
          {
            state_ = stopped;

            // Signal shutdown completion
            io_service_.post
            (
              detail::bind_handler
              (
                boost::get<0>(handler), 
                stop_error_
              )
            );
          }
        }        
      } // do_stop
      
      template <typename ConstBufferSequence, typename Handler>
      void do_write(const ConstBufferSequence& buffer, const boost::tuple<Handler>& handler)
      {  
        if (stopped == state_ || stop_in_progress == state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        } 
        else if (started != state_ || port_write_in_progress_)
        {          
          io_service_.post
          (
            detail::bind_handler
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
            serial_port_, 
            buffer, 
            strand_.wrap
            (
              make_custom_alloc_handler
              (
                write_allocator_, 
                boost::bind
                (
                  &this_type::handle_write<Handler>, 
                  shared_from_this(),
                  boost::asio::placeholders::error,
                  handler
                )
              )
            )
          );
          port_write_in_progress_ = true;
        }
      } // do_write

      template <typename Handler>
      void handle_write(const boost::system::error_code& error, boost::tuple<Handler> handler)
      {         
        port_write_in_progress_ = false;        
        io_service_.post
        (
          detail::bind_handler
          (
            boost::get<0>(handler), 
            error
          )
        );
        if (stop_in_progress == state_ && !port_read_in_progress_)
        {
          state_ = stopped;
          // Signal shutdown completion
          stop_handler_.post(stop_error_);
        }
      } 

      template <typename Handler>
      void do_read(message_ptr& message, const boost::tuple<Handler>& handler)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_aborted
            )
          );          
        }
        else if (started != state_ || read_handler_.has_target())
        {          
          io_service_.post
          (
            detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::asio::error::operation_not_supported
            )
          );
        }
        else if (!message_queue_.empty())
        {
          message = message_queue_.front();
          message_queue_.pop_front();
          io_service_.post
          (
            detail::bind_handler
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
            detail::bind_handler
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
          if (!port_read_in_progress_)
          {
            read_until_head();
          }

          // Wait for the ready message
          read_handler_.store(message, boost::get<0>(handler));          
        }        
      } // do_read                     

      void read_until_head();
      void read_until_tail();
      void handle_read_head(const boost::system::error_code& error, const std::size_t bytes_transferred);
      void handle_read_tail(const boost::system::error_code& error, const std::size_t bytes_transferred);
      
      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;
      boost::asio::serial_port serial_port_;      
      ma::handler_storage2<boost::system::error_code, message_ptr&> read_handler_;
      ma::handler_storage<boost::system::error_code> stop_handler_;      
      boost::circular_buffer<message_ptr> message_queue_;      
      boost::system::error_code read_error_;
      boost::system::error_code stop_error_;
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