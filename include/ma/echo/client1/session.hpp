//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_HPP
#define MA_ECHO_CLIENT1_SESSION_HPP

#include <stdexcept>
#include <string>
#include <boost/utility.hpp>
#include <boost/array.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {
      typedef std::string message_type;    
      class   session;
      typedef boost::shared_ptr<message_type> message_ptr;
      typedef boost::shared_ptr<session>      session_ptr;      

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
          std::size_t min_message_size_;
          std::size_t max_message_size_;
          std::size_t max_messages_;        

          explicit settings(std::size_t min_message_size, 
            std::size_t max_message_size, 
            std::size_t max_messages)
            : min_message_size_(min_message_size)
            , max_message_size_(max_message_size)
            , max_messages_(max_messages)
          {
            if (1 > min_message_size)
            {
              boost::throw_exception(std::invalid_argument("too small min_message_size"));
            }
            if (1 > max_messages)
            {
              boost::throw_exception(std::invalid_argument("too small max_messages"));
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
          , state_(ready_to_start)
          , socket_write_in_progress_(false)
          , socket_read_in_progress_(false) 
          , writed_message_queue_(settings.max_messages_)
          , read_buffer_(new char[settings.max_message_size_])
        {
        }

        ~session()
        {        
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
        void do_start(boost::tuple<Handler> handler)
        {
          if (stopped == state_ || stop_in_progress == state_)
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
          else if (ready_to_start != state_)
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
            state_ = started;          
            //todo
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
          if (stopped == state_ || stop_in_progress == state_)
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
          else
          {
            // Start shutdown
            state_ = stop_in_progress;
            // Do shutdown - abort outer operations
            wait_handler_.cancel();
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
                boost::asio::detail::bind_handler
                (
                  boost::get<0>(handler), 
                  stop_error_
                )
              );
            }
            else
            {
              stop_handler_.store(
                boost::asio::error::operation_aborted,                        
                boost::get<0>(handler));
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
        void do_wait(boost::tuple<Handler> handler)
        {
          if (stopped == state_ || stop_in_progress == state_)
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
          else if (started != state_)
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
          else if (!socket_read_in_progress_ && !socket_write_in_progress_)
          {
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                error_
              )
            );
          }
          else
          {          
            wait_handler_.store(
              boost::asio::error::operation_aborted,                        
              boost::get<0>(handler));
          } 
        } // do_wait
        
        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::socket socket_;
        ma::handler_storage<boost::system::error_code> wait_handler_;
        ma::handler_storage<boost::system::error_code> stop_handler_;
        boost::system::error_code error_;
        boost::system::error_code stop_error_;
        state_type state_;
        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        boost::circular_buffer<message_ptr> writed_message_queue_;
        boost::scoped_array<char> read_buffer_;
        handler_allocator<640> write_allocator_;
        handler_allocator<256> read_allocator_;
      }; // class session

    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_HPP
