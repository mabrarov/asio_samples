//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_HPP
#define MA_ECHO_SERVER_SESSION_HPP

#include <stdexcept>
#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/cyclic_buffer.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      class session;
      typedef boost::shared_ptr<session> session_ptr;

      class session 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session>
      {
      private:
        typedef session this_type;
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };        
        
      public:
        struct settings
        {              
          std::size_t buffer_size_;
          int socket_recv_buffer_size_;
          int socket_send_buffer_size_;
          bool no_delay_;

          explicit settings(std::size_t buffer_size,
            int socket_recv_buffer_size,
            int socket_send_buffer_size,
            bool no_delay)
            : buffer_size_(buffer_size)         
            , socket_recv_buffer_size_(socket_recv_buffer_size)
            , socket_send_buffer_size_(socket_send_buffer_size)
            , no_delay_(no_delay)
          {
            if (1 > buffer_size)
            {
              boost::throw_exception(std::invalid_argument("too small buffer_size"));
            }
            if (0 > socket_recv_buffer_size)
            {
              boost::throw_exception(std::invalid_argument("socket_recv_buffer_size must be non negative"));
            }
            if (0 > socket_send_buffer_size)
            {
              boost::throw_exception(std::invalid_argument("socket_send_buffer_size must be non negative"));
            }
          }
        }; // struct settings

        explicit session(boost::asio::io_service& io_service,
          const settings& settings)
          : io_service_(io_service)
          , strand_(io_service)
          , socket_(io_service)
          , wait_handler_(io_service)
          , stop_handler_(io_service)
          , settings_(settings)
          , state_(ready_to_start)
          , socket_write_in_progress_(false)
          , socket_read_in_progress_(false) 
          , buffer_(settings.buffer_size_)
        {          
        }

        ~session()
        {        
        }

        void reset()
        {
          boost::system::error_code ignored;
          socket_.close(ignored);
          error_ = stop_error_ = boost::system::error_code();          
          state_ = ready_to_start;
          buffer_.reset();          
        }
        
        boost::asio::ip::tcp::socket& socket()
        {
          return socket_;
        }
        
        template <typename Handler>
        void async_start(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
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
        } // async_start

        template <typename Handler>
        void async_stop(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
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
        } // async_stop

        template <typename Handler>
        void async_wait(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
            (
              handler, 
              boost::bind
              (
                &this_type::do_wait<Handler>,
                shared_from_this(),
                boost::make_tuple(handler)
              )
            )
          );  
        } // async_wait
        
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
            boost::system::error_code start_error;
            using boost::asio::ip::tcp;
            socket_.set_option(tcp::socket::receive_buffer_size(settings_.socket_recv_buffer_size_), start_error);
            if (!start_error)
            {
              socket_.set_option(tcp::socket::send_buffer_size(settings_.socket_recv_buffer_size_), start_error);
              if (!start_error)
              {
                if (settings_.no_delay_)
                {
                  socket_.set_option(tcp::no_delay(true), start_error);
                }
                if (!start_error) 
                {
                  state_ = started;          
                  read_some();
                }
              }
            }                        
            io_service_.post
            (
              detail::bind_handler
              (
                boost::get<0>(handler), 
                start_error
              )
            ); 
          }
        } // do_start

        template <typename Handler>
        void do_stop(const boost::tuple<Handler>& handler)
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
            else
            {
              stop_handler_.store(boost::get<0>(handler));
            }
          }
        } // do_stop

        bool may_complete_stop() const
        {
          return !socket_write_in_progress_ && !socket_read_in_progress_;
        }

        void complete_stop()
        {        
          boost::system::error_code error;
          socket_.close(error);
          if (!stop_error_)
          {
            stop_error_ = error;
          }
          state_ = stopped;  
        }      

        template <typename Handler>
        void do_wait(const boost::tuple<Handler>& handler)
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
          else if (started != state_)
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
          else if (!socket_read_in_progress_ && !socket_write_in_progress_)
          {
            io_service_.post
            (
              detail::bind_handler
              (
                boost::get<0>(handler), 
                error_
              )
            );
          }
          else
          {          
            wait_handler_.store(boost::get<0>(handler));
          } 
        } // do_wait

        void read_some()
        {
          ma::cyclic_buffer::mutable_buffers_type buffers(buffer_.prepared());
          std::size_t buffers_size = boost::asio::buffers_end(buffers) - 
            boost::asio::buffers_begin(buffers);
          if (buffers_size)
          {
            socket_.async_read_some
            (
              buffers,
              strand_.wrap
              (
                ma::make_custom_alloc_handler
                (
                  read_allocator_,
                  boost::bind
                  (
                    &this_type::handle_read_some,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                  )
                )
              )
            );
            socket_read_in_progress_ = true;
          }        
        }

        void write_some()
        {
          ma::cyclic_buffer::const_buffers_type buffers(buffer_.data());
          std::size_t buffers_size = boost::asio::buffers_end(buffers) - 
            boost::asio::buffers_begin(buffers);
          if (buffers_size)
          {
            socket_.async_write_some
            (
              buffers,
              strand_.wrap
              (
                ma::make_custom_alloc_handler
                (
                  write_allocator_,
                  boost::bind
                  (
                    &this_type::handle_write_some,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                  )
                )
              )
            );
            socket_write_in_progress_ = true;
          }   
        }

        void handle_read_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred)
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
        } // handle_read_some

        void handle_write_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred)
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
        } // handle_write_some

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::socket socket_;
        ma::handler_storage<boost::system::error_code> wait_handler_;
        ma::handler_storage<boost::system::error_code> stop_handler_;
        boost::system::error_code error_;
        boost::system::error_code stop_error_;
        settings settings_;
        state_type state_;
        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        ma::cyclic_buffer buffer_;
        in_place_handler_allocator<640> write_allocator_;
        in_place_handler_allocator<256> read_allocator_;
      }; // class session

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_HPP
