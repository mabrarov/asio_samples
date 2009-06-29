//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_DETAIL_SESSION_HPP
#define MA_ECHO_DETAIL_SESSION_HPP

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
  namespace echo
  {  
    namespace detail
    {  
      template <typename AsyncStream>
      class session 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session<AsyncStream> >
      {
      public:
        typedef std::size_t size_type;        

      private:
        typedef handler_storage<boost::system::error_code> handler_storage_type;
        typedef session<AsyncStream> this_type;              

      public:      
        typedef AsyncStream next_layer_type;
        typedef typename next_layer_type::lowest_layer_type lowest_layer_type;
        typedef this_type* raw_ptr;     
        typedef boost::shared_ptr<this_type> shared_ptr;

        raw_ptr prev_;
        shared_ptr next_;    
        handler_allocator service_handler_allocator_;

        explicit session(
          boost::asio::io_service& io_service,
          const size_type read_buffer_size)
          : prev_(0)
          , next_()                    
          , pending_calls_(0)
          , io_service_(io_service)
          , strand_(io_service)
          , stream_(io_service)                    
          , handshake_done_(false)          
          , writing_(false)
          , reading_(false)          
        {
          //todo
        }

        ~session()
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

        void prepare_for_destruction()
        {
          // Close all user operations.          
          if (wait_handler_)
          {
            wait_handler_(boost::asio::error::operation_aborted);
          }          
          if (shutdown_handler_)
          {
            shutdown_handler_(boost::asio::error::operation_aborted);
          }
        }               

        template <typename Handler>
        void async_handshake(Handler handler)
        {        
          ++pending_calls_;
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_handshake<Handler>, shared_from_this(), boost::make_tuple(handler))));
        }

        template <typename Handler>
        void async_shutdown(Handler handler)
        {
          ++pending_calls_;
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_shutdown<Handler>, shared_from_this(), boost::make_tuple(handler))));
        }

        template <typename Handler>
        void async_wait(Handler handler)
        {        
          ++pending_calls_;
          strand_.dispatch(make_context_alloc_handler(handler,
            boost::bind(&this_type::start_wait<Handler>, shared_from_this(), boost::make_tuple(handler))));
        }        

      private:        
        template <typename Handler>
        void start_handshake(boost::tuple<Handler> handler)
        {         
          --pending_calls_;
          if (shutdown_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
            if (!pending_calls_)
            {
              handshake_done_ = false;            
              shutdown_handler_(shutdown_error_);
            }
          }
          else if (handshake_done_)          
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_open));
          }
          else
          {  
            //todo
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::system::error_code()));              
          }
        }

        template <typename Handler>
        void start_shutdown(boost::tuple<Handler> handler)
        {  
          --pending_calls_;
          if (shutdown_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_started));
            if (!pending_calls_)
            {
              handshake_done_ = false;            
              shutdown_handler_(shutdown_error_);
            }
          }          
          else 
          {
            if (wait_handler_)
            {
              read_handler_(boost::asio::error::operation_aborted);
            }            
            stream_.close(shutdown_error_);
            if (!pending_calls_)
            {
              handshake_done_ = false;
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
        void start_wait(boost::tuple<Handler> handler)
        {          
          --pending_calls_;
          if (shutdown_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_aborted));
            if (!pending_calls_)
            {
              handshake_done_ = false;            
              shutdown_handler_(shutdown_error_);
            }
          }
          else if (!handshake_done_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::not_connected));
          }
          else if (wait_handler_)
          {
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::already_started));
          }          
          else
          {
            //todo          
            io_service_.post(boost::asio::detail::bind_handler(
              boost::get<0>(handler), boost::asio::error::operation_not_supported));
          }
        }

        void handle_read(const boost::system::error_code& error, const std::size_t bytes_transferred)
        {          
          reading_ = false;
          --pending_calls_;
          if (shutdown_handler_ && !pending_calls_)
          {
            handshake_done_ = false;            
            shutdown_handler_(shutdown_error_);          
          }          
          else if (error)
          {
            //todo
          }
          else
          {
            //todo
          }          
        }
        
        boost::detail::atomic_count pending_calls_;
        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;
        next_layer_type stream_;
        handler_storage_type shutdown_handler_;
        handler_storage_type wait_handler_;        
        bool handshake_done_;        
        bool writing_;
        bool reading_;        
        handler_allocator write_handler_allocator_;
        handler_allocator read_handler_allocator_;
        boost::system::error_code shutdown_error_;              
      }; // class session

    } // namespace detail
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_DETAIL_SESSION_HPP
