//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <utility>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)      
      class session::io_handler_binder
      {
      private:
        typedef io_handler_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void result_type;
        typedef void (session::*function_type)(const boost::system::error_code&, const std::size_t);

        template <typename SessionPtr>
        io_handler_binder(function_type function, SessionPtr&& the_session)
          : function_(function)
          , session_(std::forward<SessionPtr>(the_session))
        {
        }         

        io_handler_binder(this_type&& other)
          : function_(other.function_)
          , session_(std::move(other.session_))
        {
        }

        void operator()(const boost::system::error_code& error, 
          const std::size_t bytes_transferred)
        {
          ((*session_).*function_)(error, bytes_transferred);
        }

      private:
        function_type function_;
        session_ptr session_;
      }; // class session::io_handler_binder
#endif // defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

      session::session(boost::asio::io_service& io_service, 
        const session_config& config)
        : socket_write_in_progress_(false)
        , socket_read_in_progress_(false) 
        , state_(ready_to_start)
        , io_service_(io_service)
        , strand_(io_service)
        , socket_(io_service)
        , wait_handler_(io_service)
        , stop_handler_(io_service)
        , config_(config)                
        , buffer_(config.buffer_size)
      {          
      }      

      void session::reset()
      {
        boost::system::error_code ignored;
        socket_.close(ignored);
        wait_error_.clear();
        stop_error_.clear();          
        state_ = ready_to_start;
        buffer_.reset();          
      }
              
      boost::system::error_code session::start()
      {                
        if (ready_to_start != state_)
        {
          return server_error::invalid_state;
        }
        boost::system::error_code error;
        using boost::asio::ip::tcp;        
        if (config_.socket_recv_buffer_size)
        {
          socket_.set_option(
            tcp::socket::receive_buffer_size(*config_.socket_recv_buffer_size),
            error);
        }
        if (!error)
        {
          if (config_.socket_recv_buffer_size)
          {
            socket_.set_option(
              tcp::socket::send_buffer_size(*config_.socket_recv_buffer_size), 
              error);
          }
          if (!error)
          {          
            if (config_.no_delay)
            {
              socket_.set_option(tcp::no_delay(*config_.no_delay), error);
            }
          }
        }
        if (error)
        {
          boost::system::error_code ignored;
          socket_.close(ignored);          
        }
        else 
        {        
          state_ = started;          
          read_some();   
        }
        return error;
      }

      boost::optional<boost::system::error_code> session::stop()
      {        
        if (stopped == state_ || stop_in_progress == state_)
        {          
          return boost::system::error_code(server_error::invalid_state);
        }        
        // Start shutdown
        state_ = stop_in_progress;
        // Do shutdown - abort outer operations
        if (wait_handler_.has_target())
        {
          wait_handler_.post(server_error::operation_aborted);
        }
        // Do shutdown - flush socket's write_some buffer
        if (!socket_write_in_progress_) 
        {
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, 
            stop_error_);
        }          
        // Check for shutdown continuation          
        if (may_complete_stop())
        {
          complete_stop();
          return stop_error_;          
        }
        return boost::optional<boost::system::error_code>();
      }

      boost::optional<boost::system::error_code> session::wait()
      {                
        if (started != state_ || wait_handler_.has_target())
        {
          return boost::system::error_code(server_error::invalid_state);
        }
        if (!socket_read_in_progress_ && !socket_write_in_progress_)
        {
          return wait_error_;
        }
        return boost::optional<boost::system::error_code>();
      }

      bool session::may_complete_stop() const
      {
        return !socket_write_in_progress_ && !socket_read_in_progress_;
      }

      void session::complete_stop()
      {        
        boost::system::error_code error;
        socket_.close(error);
        if (!stop_error_)
        {
          stop_error_ = error;
        }
        state_ = stopped;  
      }

      void session::read_some()
      {
        cyclic_buffer::mutable_buffers_type buffers(buffer_.prepared());
        std::size_t buffers_size = std::distance(
          boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

        if (buffers_size)
        {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
          socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_, 
            make_custom_alloc_handler(read_allocator_,
              io_handler_binder(&this_type::handle_read_some, shared_from_this()))));
#else
          socket_.async_read_some(buffers, MA_STRAND_WRAP(strand_, 
            make_custom_alloc_handler(read_allocator_,
              boost::bind(&this_type::handle_read_some, shared_from_this(), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
#endif
          socket_read_in_progress_ = true;
        }        
      }

      void session::write_some()
      {
        cyclic_buffer::const_buffers_type buffers(buffer_.data());
        std::size_t buffers_size = std::distance(
          boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

        if (buffers_size)
        {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
          socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_, 
            make_custom_alloc_handler(write_allocator_,
              io_handler_binder(&this_type::handle_write_some, shared_from_this()))));
#else
          socket_.async_write_some(buffers, MA_STRAND_WRAP(strand_,
            make_custom_alloc_handler(write_allocator_,
              boost::bind(&this_type::handle_write_some, shared_from_this(), 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred))));
#endif
          socket_write_in_progress_ = true;
        }   
      }

      void session::handle_read_some(
        const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {        
        socket_read_in_progress_ = false;
        // Check for pending session stop operation 
        if (stop_in_progress == state_)
        {  
          if (may_complete_stop())
          {
            complete_stop();                   
            post_stop_handler();
          }
          return;
        }
        if (error)
        {
          if (!wait_error_)
          {
            wait_error_ = error;
          }  
          if (wait_handler_.has_target())
          {
            wait_handler_.post(wait_error_);
          }          
          return;
        }        
        buffer_.consume(bytes_transferred);
        read_some();
        if (!socket_write_in_progress_)
        {
          write_some();
        }
      }

      void session::handle_write_some(
        const boost::system::error_code& error, 
        const std::size_t bytes_transferred)
      {
        socket_write_in_progress_ = false;
        // Check for pending session manager stop operation 
        if (stop_in_progress == state_)
        {
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, stop_error_);
          if (may_complete_stop())
          {
            complete_stop();  
            post_stop_handler();
          }
          return;
        }
        if (error)
        {
          if (!wait_error_)
          {
            wait_error_ = error;
          }                    
          if (wait_handler_.has_target())
          {
            wait_handler_.post(wait_error_);
          }
          return;
        }        
        buffer_.commit(bytes_transferred);
        write_some();
        if (!socket_read_in_progress_)
        {
          read_some();
        }        
      }

      void session::post_stop_handler()
      {
        if (stop_handler_.has_target()) 
        {
          // Signal shutdown completion
          stop_handler_.post(stop_error_);
        }
      }
        
    } // namespace server
  } // namespace echo
} // namespace ma
