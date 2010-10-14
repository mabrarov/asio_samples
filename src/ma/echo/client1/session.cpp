//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/client1/session.hpp>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {
      session::session(boost::asio::io_service& io_service, const session_config& config)
        : socket_write_in_progress_(false)
        , socket_read_in_progress_(false) 
        , state_(ready_to_start)
        , io_service_(io_service)
        , strand_(io_service)
        , socket_(io_service)
        , wait_handler_(io_service)
        , stop_handler_(io_service)
        , config_(config)                
        , buffer_(config.buffer_size_)
      {          
      } // session::session      

      void session::reset()
      {
        boost::system::error_code ignored;
        socket_.close(ignored);
        error_ = stop_error_ = boost::system::error_code();          
        state_ = ready_to_start;
        buffer_.reset();          
      } // session::reset             

      void session::start(boost::system::error_code& error)
      {        
        if (stopped == state_ || stop_in_progress == state_)
        {     
          error = boost::asio::error::operation_aborted;          
        } 
        else if (ready_to_start != state_)
        {          
          error = boost::asio::error::operation_not_supported;                      
        }
        else
        {
          using boost::asio::ip::tcp;
          socket_.set_option(tcp::socket::receive_buffer_size(config_.socket_recv_buffer_size_), error);
          if (!error)
          {
            socket_.set_option(tcp::socket::send_buffer_size(config_.socket_recv_buffer_size_), error);
            if (!error)
            {
              if (config_.no_delay_)
              {
                socket_.set_option(tcp::no_delay(true), error);
              }
              if (!error) 
              {
                state_ = started;          
                read_some();
              }
            }
          }           
        }        
      } // session::start

      void session::stop(boost::system::error_code& error, bool& completed)
      {
        completed = true;
        if (stopped == state_ || stop_in_progress == state_)
        {          
          error = boost::asio::error::operation_aborted;                      
        }
        else
        {          
          // Start shutdown
          state_ = stop_in_progress;
          // Do shutdown - abort outer operations
          if (wait_handler_.has_target())
          {
            wait_handler_.post(boost::asio::error::operation_aborted);
          }
          // Do shutdown - flush socket's write_some buffer
          if (!socket_write_in_progress_) 
          {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, stop_error_);            
          }          
          // Check for shutdown continuation          
          if (may_complete_stop())
          {
            complete_stop();
            error = stop_error_;
            completed = true;
          }
          else 
          {
            completed = false;
          }
        }        
      } // session::stop

      void session::wait(boost::system::error_code& error, bool& completed)
      {
        completed = true;
        if (stopped == state_ || stop_in_progress == state_)
        {          
          error = boost::asio::error::operation_aborted;
        } 
        else if (started != state_)
        {
          error = boost::asio::error::operation_not_supported;
        }
        else if (!socket_read_in_progress_ && !socket_write_in_progress_)
        {
          error = error_;            
        }
        else if (wait_handler_.has_target())
        {
          error = boost::asio::error::operation_not_supported;
        }
        else
        {
          completed = false;
        } 
      } // session::wait

      bool session::may_complete_stop() const
      {
        return !socket_write_in_progress_ && !socket_read_in_progress_;
      } // session::may_complete_stop

      void session::complete_stop()
      {        
        boost::system::error_code error;
        socket_.close(error);
        if (!stop_error_)
        {
          stop_error_ = error;
        }
        state_ = stopped;  
      } // session::complete_stop

      void session::read_some()
      {
        cyclic_buffer::mutable_buffers_type buffers(buffer_.prepared());
        std::size_t buffers_size = boost::asio::buffers_end(buffers) - 
          boost::asio::buffers_begin(buffers);
        if (buffers_size)
        {
          socket_.async_read_some(buffers, strand_.wrap(make_custom_alloc_handler(read_allocator_,
            boost::bind(&this_type::handle_read_some, shared_from_this(), 
              boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
          socket_read_in_progress_ = true;
        }        
      } // session::read_some

      void session::write_some()
      {
        cyclic_buffer::const_buffers_type buffers(buffer_.data());
        std::size_t buffers_size = boost::asio::buffers_end(buffers) - 
          boost::asio::buffers_begin(buffers);
        if (buffers_size)
        {
          socket_.async_write_some(buffers, strand_.wrap(make_custom_alloc_handler(write_allocator_,
            boost::bind(&this_type::handle_write_some, shared_from_this(), 
              boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
          socket_write_in_progress_ = true;
        }   
      } // session::write_some

      void session::handle_read_some(const boost::system::error_code& error, const std::size_t bytes_transferred)
      {        
        socket_read_in_progress_ = false;
        if (stop_in_progress == state_)
        {  
          if (may_complete_stop())
          {
            complete_stop();       
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else if (error)
        {
          if (!error_)
          {
            error_ = error;
          }                    
          wait_handler_.post(error);
        }
        else 
        {
          buffer_.consume(bytes_transferred);
          read_some();
          if (!socket_write_in_progress_)
          {
            write_some();
          }
        }
      } // session::handle_read_some

      void session::handle_write_some(const boost::system::error_code& error, const std::size_t bytes_transferred)
      {
        socket_write_in_progress_ = false;
        if (stop_in_progress == state_)
        {
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, stop_error_);
          if (may_complete_stop())
          {
            complete_stop();       
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else if (error)
        {
          if (!error_)
          {
            error_ = error;
          }                    
          wait_handler_.post(error);
        }
        else
        {
          buffer_.commit(bytes_transferred);
          write_some();
          if (!socket_read_in_progress_)
          {
            read_some();
          }
        }
      } // session::handle_write_some
        
    } // namespace client1
  } // namespace echo
} // namespace ma
