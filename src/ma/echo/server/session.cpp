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
      namespace 
      {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)      
        class io_handler_binder
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

  #if defined(_DEBUG)
          io_handler_binder(const this_type& other)
            : function_(other.function_)
            , session_(other.session_)
          {
          }
  #endif

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
        }; // class io_handler_binder

        class timer_handler_binder
        {
        private:
          typedef timer_handler_binder this_type;
          this_type& operator=(const this_type&);

        public:
          typedef void result_type;
          typedef void (session::*function_type)(const boost::system::error_code&);

          template <typename SessionPtr>
          timer_handler_binder(function_type function, SessionPtr&& the_session)
            : function_(function)
            , session_(std::forward<SessionPtr>(the_session))
          {
          } 

  #if defined(_DEBUG)
          timer_handler_binder(const this_type& other)
            : function_(other.function_)
            , session_(other.session_)
          {
          }
  #endif

          timer_handler_binder(this_type&& other)
            : function_(other.function_)
            , session_(std::move(other.session_))
          {
          }

          void operator()(const boost::system::error_code& error)
          {
            ((*session_).*function_)(error);
          }

        private:
          function_type function_;
          session_ptr session_;
        }; // class timer_handler_binder
#endif // defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

      } // anonymous namespace

      session::session(boost::asio::io_service& io_service, const session_options& options)
        : socket_recv_buffer_size_(options.socket_recv_buffer_size())
        , socket_send_buffer_size_(options.socket_send_buffer_size())
        , no_delay_(options.no_delay())
        , inactivity_timeout_(options.inactivity_timeout())
        , socket_write_in_progress_(false)
        , socket_read_in_progress_(false)
        , timer_wait_in_progress_(false)
        , timer_cancelled_(false)
        , socket_closed_for_stop_(false)
        , external_state_(ready_to_start)
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
        boost::system::error_code ignored;
        socket_.close(ignored);

        socket_write_in_progress_ = false;
        socket_read_in_progress_  = false;
        timer_wait_in_progress_   = false;
        timer_cancelled_          = false;
        socket_closed_for_stop_   = false;

        wait_error_.clear();
        stop_error_.clear();
        buffer_.reset();

        external_state_ = ready_to_start;        
      }
              
      boost::system::error_code session::start()
      {                
        if (ready_to_start != external_state_)
        {
          return server_error::invalid_state;
        }
        boost::system::error_code error = apply_socket_options();
        if (error)
        {
          boost::system::error_code ignored;
          socket_.close(ignored);
        }
        else 
        {
          external_state_ = started;
          continue_work();
        }
        return error;
      }

      boost::optional<boost::system::error_code> session::stop()
      {        
        if (stopped == external_state_ || stop_in_progress == external_state_)
        {          
          return boost::system::error_code(server_error::invalid_state);
        }
        // Start shutdown
        external_state_ = stop_in_progress;
        // Do shutdown - abort outer operations        
        if (wait_handler_.has_target())
        {
          wait_handler_.post(server_error::operation_aborted);
        }
        // Do shutdown - flush socket's write buffer        
        boost::system::error_code shutdown_error;
        shutdown_socket_send(shutdown_error);
        set_stop_error(shutdown_error);
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
        if (started != external_state_ || wait_handler_.has_target())
        {
          return boost::system::error_code(server_error::invalid_state);
        }
        if (wait_error_)
        {
          return wait_error_;
        }
        return boost::optional<boost::system::error_code>();
      }

      boost::system::error_code session::apply_socket_options()
      {
        typedef protocol_type::socket socket_type;
        boost::system::error_code error;
        if (socket_recv_buffer_size_)
        {
          socket_.set_option(socket_type::receive_buffer_size(*socket_recv_buffer_size_), error);
          if (error)
          {
            return error;
          }
        }        
        if (socket_recv_buffer_size_)
        {
          socket_.set_option(socket_type::send_buffer_size(*socket_recv_buffer_size_), error);
          if (error)
          {
            return error;
          }
        }
        if (no_delay_)
        {
          socket_.set_option(protocol_type::no_delay(*no_delay_), error);
        }
        return error;
      }      

      bool session::may_complete_stop() const
      {
        return !socket_write_in_progress_ && !socket_read_in_progress_ && !timer_wait_in_progress_;
      }

      void session::complete_stop()
      {
        boost::system::error_code error;
        close_socket_for_stop(error);
        if (!stop_error_)
        {
          stop_error_ = error;
        }        
        external_state_ = stopped;  
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

      void session::shutdown_socket_send(boost::system::error_code& error)
      {
        boost::system::error_code local_error;
        if (!socket_closed_for_stop_)
        {
          socket_.shutdown(protocol_type::socket::shutdown_send, local_error);
        }
        error = local_error;
      }

      void session::close_socket_for_stop(boost::system::error_code& error)
      {
        boost::system::error_code local_error;
        if (!socket_closed_for_stop_)
        {
          socket_.close(local_error);
          socket_closed_for_stop_ = true;
        }
        error = local_error;
      }

      void session::start_timer(boost::system::error_code& error)
      {
        boost::system::error_code local_error;
        if (inactivity_timeout_)
        {
          timer_.expires_from_now(*inactivity_timeout_, local_error);
          if (!local_error)
          {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
            timer_.async_wait(MA_STRAND_WRAP(strand_, 
              make_custom_alloc_handler(inactivity_allocator_,
                timer_handler_binder(&this_type::handle_timeout, shared_from_this()))));
#else
            timer_.async_wait(MA_STRAND_WRAP(strand_, 
              make_custom_alloc_handler(inactivity_allocator_,
                boost::bind(&this_type::handle_timeout, shared_from_this(), boost::asio::placeholders::error))));
#endif
            timer_wait_in_progress_ = true;
            timer_cancelled_        = false;
          }
        }
        error = local_error;
      }

      void session::cancel_timer(boost::system::error_code& error)
      {
        boost::system::error_code local_error;
        if (timer_wait_in_progress_ && !timer_cancelled_)
        {          
          timer_.cancel(local_error);
          timer_cancelled_ = true;
        }
        error = local_error;
      }

      void session::handle_read_some(const boost::system::error_code& error, std::size_t bytes_transferred)
      {        
        socket_read_in_progress_ = false;

        // Cancel inactivity timer
        boost::system::error_code local_error;
        cancel_timer(local_error);
        if (local_error)
        {
          set_wait_error(local_error);          
          close_socket_for_stop(local_error);
        }

        // Check for pending session stop operation 
        if (stop_in_progress == external_state_)
        {
          continue_stop();
          return;
        }
        // Normal control flow        
        if (error)
        {
          set_wait_error(error);
          return;
        }        
        buffer_.consume(bytes_transferred);
        continue_work();
      }

      void session::handle_write_some(const boost::system::error_code& error, std::size_t bytes_transferred)
      {
        socket_write_in_progress_ = false;
        
        // Cancel inactivity timer
        boost::system::error_code local_error;
        cancel_timer(local_error);
        if (local_error)
        {
          set_wait_error(local_error);          
          close_socket_for_stop(local_error);
        }

        // Check for pending session manager stop operation 
        if (stop_in_progress == external_state_)
        {
          boost::system::error_code shutdown_error;
          shutdown_socket_send(shutdown_error);
          if (shutdown_error && !stop_error_)
          {
            stop_error_ = shutdown_error;
          }
          continue_stop();
          return;
        }
        // Normal control flow
        if (error)
        {
          set_wait_error(error);
          return;
        }        
        buffer_.commit(bytes_transferred);
        continue_work();
      }

      void session::handle_timeout(const boost::system::error_code& error)
      {
        timer_wait_in_progress_ = false;
        // Check for pending session stop operation 
        if (stop_in_progress == external_state_)
        {
          if (!timer_cancelled_)
          {
            // Timer was normally fired during stop
            boost::system::error_code ignored;
            close_socket_for_stop(ignored);            
          }
          continue_stop();
          return;
        }        
        if (timer_cancelled_)
        {
          // Normal timer cancellation
          continue_work();
          return;
        }
        if (error)
        {
          // Abnormal timer behaivour
          set_wait_error(error);
        }
        else 
        {
          // Timer was normally fired 
          set_wait_error(server_error::inactivity_timeout);
        }
        boost::system::error_code ignored;
        close_socket_for_stop(ignored);
      }

      void session::continue_work()
      {
        if (!timer_wait_in_progress_ && !wait_error_)
        {                  
          if (!socket_read_in_progress_)
          {
            read_some();
          }
          if (!socket_write_in_progress_)
          {
            write_some();
          }
          if (socket_read_in_progress_ || socket_write_in_progress_)
          {
            boost::system::error_code local_error;
            start_timer(local_error);
            if (local_error)
            {
              set_wait_error(local_error);              
              close_socket_for_stop(local_error);
            }
          }
        }
      }

      void session::continue_stop()
      {
        if (may_complete_stop())
        {
          complete_stop();
          if (stop_handler_.has_target()) 
          {
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
      }

      void session::set_wait_error(const boost::system::error_code& error)
      {
        if (!wait_error_)
        {
          wait_error_ = error;
          if (wait_handler_.has_target())
          {
            wait_handler_.post(wait_error_);
          }
        }
      }

      void session::set_stop_error(const boost::system::error_code& error)
      {
        if (!stop_error_)
        {
          stop_error_ = error;
        }
      }            
        
    } // namespace server
  } // namespace echo
} // namespace ma
