//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_HPP
#define MA_ECHO_SERVER_SESSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      class session 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session>
      {
      private:
        typedef session this_type;        
        
      public:
        typedef boost::asio::ip::tcp protocol_type;

        session(boost::asio::io_service& io_service, const session_config& config);

        ~session()
        {
        }

        protocol_type::socket& socket()
        {
          return socket_;
        }

        void reset();                
        
#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        template <typename Handler>
        void async_start(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),  
            forward_handler_binder<handler_type>(&this_type::do_start<handler_type>, shared_from_this())));  
        }

        template <typename Handler>
        void async_stop(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
            forward_handler_binder<handler_type>(&this_type::do_stop<handler_type>, shared_from_this()))); 
        }

        template <typename Handler>
        void async_wait(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
            forward_handler_binder<handler_type>(&this_type::do_wait<handler_type>, shared_from_this())));  
        }
#else
        template <typename Handler>
        void async_start(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),  
            boost::bind(&this_type::do_start<handler_type>, shared_from_this(), _1)));  
        }

        template <typename Handler>
        void async_stop(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
            boost::bind(&this_type::do_stop<handler_type>, shared_from_this(), _1))); 
        }

        template <typename Handler>
        void async_wait(Handler&& handler)
        {
          typedef typename ma::remove_cv_reference<Handler>::type handler_type;
          strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
            boost::bind(&this_type::do_wait<handler_type>, shared_from_this(), _1)));  
        }
#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)
        template <typename Handler>
        void async_start(const Handler& handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_start<Handler>, shared_from_this(), _1)));
        }

        template <typename Handler>
        void async_stop(const Handler& handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_stop<Handler>, shared_from_this(), _1)));
        }

        template <typename Handler>
        void async_wait(const Handler& handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_wait<Handler>, shared_from_this(), _1)));
        }
#endif // defined(MA_HAS_RVALUE_REFS)
        
      private:

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        class io_handler_binder; 

        template <typename Arg>
        class forward_handler_binder
        {
        private:
          typedef forward_handler_binder this_type;
          this_type& operator=(const this_type&);

        public:
          typedef void result_type;
          typedef void (session::*function_type)(const Arg&);

          template <typename SessionPtr>
          forward_handler_binder(function_type function, SessionPtr&& the_session)
            : function_(function)
            , session_(std::forward<SessionPtr>(the_session))
          {
          }

#if defined(_DEBUG)
          forward_handler_binder(const this_type& other)
            : function_(other.function_)
            , session_(other.session_)
          {
          }
#endif

          forward_handler_binder(this_type&& other)
            : function_(other.function_)
            , session_(std::move(other.session_))
          {
          }

          void operator()(const Arg& arg)
          {
            ((*session_).*function_)(arg);
          }

        private:
          function_type function_;
          session_ptr session_;          
        }; // class forward_handler_binder
#endif // defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        }; // enum state_type

        template <typename Handler>
        void do_start(const Handler& handler)
        {
          boost::system::error_code result = start();
          io_service_.post(detail::bind_handler(handler, result));
        }

        template <typename Handler>
        void do_stop(const Handler& handler)
        {
          if (boost::optional<boost::system::error_code> result = stop())
          {
            io_service_.post(detail::bind_handler(handler, *result));
          }
          else
          {
            stop_handler_.put(handler);            
          }
        }

        template <typename Handler>
        void do_wait(const Handler& handler)
        {
          if (boost::optional<boost::system::error_code> result = wait())
          {
            io_service_.post(detail::bind_handler(handler, *result));
          } 
          else
          {
            wait_handler_.put(handler);
          }
        }

        boost::system::error_code start();
        boost::optional<boost::system::error_code> stop();
        boost::optional<boost::system::error_code> wait();

        bool may_complete_stop() const;
        void complete_stop();

        void read_some();
        void write_some();
        void handle_read_some(const boost::system::error_code& error, const std::size_t bytes_transferred);
        void handle_write_some(const boost::system::error_code& error, const std::size_t bytes_transferred);
        void post_stop_handler();
        
        session_config::optional_int  socket_recv_buffer_size_;
        session_config::optional_int  socket_send_buffer_size_;
        session_config::optional_bool no_delay_;

        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        state_type state_;

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;
        protocol_type::socket socket_;
        cyclic_buffer buffer_;

        handler_storage<boost::system::error_code> wait_handler_;
        handler_storage<boost::system::error_code> stop_handler_;
        boost::system::error_code wait_error_;
        boost::system::error_code stop_error_;

        in_place_handler_allocator<640> write_allocator_;
        in_place_handler_allocator<256> read_allocator_;
      }; // class session

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_HPP
