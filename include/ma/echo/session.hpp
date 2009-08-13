//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SESSION_HPP
#define MA_ECHO_SESSION_HPP

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
  namespace echo
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
      explicit session(boost::asio::io_service& io_service)
        : io_service_(io_service)
        , strand_(io_service)
        , socket_(io_service)
        , wait_handler_(io_service)
        , stop_handler_(io_service)
        , state_(ready_to_start)
        , socket_write_in_progress_(false)
        , socket_read_in_progress_(false)
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
          read_some();
          io_service_.post
          (
            boost::asio::detail::bind_handler
            (
              boost::get<0>(handler), 
              boost::system::error_code()
            )
          ); 
        }
      }      

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

          // Do shutdown - abort inner operations          
          socket_.close(stop_error_);          
          
          // Do shutdown - abort outer operations
          wait_handler_.cancel();

          // Check for shutdown continuation
          if (socket_read_in_progress_ || socket_write_in_progress_)
          {
            stop_handler_.store(
              boost::asio::error::operation_aborted,                        
              boost::get<0>(handler));
          }
          else
          {                        
            state_ = stopped;          
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
      }

      void read_some()
      {
        //todo
      }

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
      in_place_handler_allocator<256> write_allocator_;
      in_place_handler_allocator<256> read_allocator_;
    }; // class session

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SESSION_HPP
