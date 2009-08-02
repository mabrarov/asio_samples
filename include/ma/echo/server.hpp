//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_HPP
#define MA_ECHO_SERVER_HPP

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/echo/session.hpp>

namespace ma
{    
  namespace echo
  {    
    class server;
    typedef boost::shared_ptr<server> server_ptr;

    class server 
      : private boost::noncopyable 
      , public boost::enable_shared_from_this<server>
    {
    private:
      typedef server this_type;      
      struct session_state;
      typedef boost::shared_ptr<session_state> session_state_ptr;
      typedef boost::weak_ptr<session_state> session_state_weak_ptr;

      struct session_state : private boost::noncopyable
      {        
        session_state_weak_ptr prev_;
        session_state_ptr next_;
        session_ptr session_;
        bool stop_in_progress_;

        explicit session_state(const session_ptr& session)
          : session_(session)
          , stop_in_progress_(false)
        {
        }
      }; // session_state

      class session_state_set : private boost::noncopyable
      {
      public:
        explicit session_state_set()
        {
        }

        void insert(const session_state_ptr& session_state)
        {
          session_state->next_ = front_;
          session_state->prev_.reset();
          if (front_)
          {
            front_->prev_ = session_state;
          }
          front_ = session_state;
        }

        void erase(const session_state_ptr& session_state)
        {
          if (front_ == session_state)
          {
            front_ = front_->next_;
          }
          session_state::pointer prev = session_state->prev_.lock();
          if (prev)
          {
            prev->next_ = session_state->next_;
          }
          if (session_state->next_)
          {
            session_state->next_->prev_ = prev;
          }
          session_state->prev_.reset();
          session_state->next_.reset();
        }

      private:
        session_state_ptr front_;
      }; // session_state_set

    public:
      explicit server(boost::asio::io_service& io_service,
        boost::asio::io_service& session_io_service)
        : io_service_(io_service)
        , session_io_service_(session_io_service)
        , strand_(io_service)
        , acceptor_(io_service)
      {        
      }

      ~server()
      {        
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
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            boost::asio::error::operation_not_supported
          )
        );

        session_ptr session(new session(session_io_service_));
        session_state_ptr session_state(new session_state(session));
        sessions_.insert(session_state);

      } // do_start

      template <typename Handler>
      void do_stop(boost::tuple<Handler> handler)
      {
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            boost::asio::error::operation_not_supported
          )
        );
      } // do_stop

      template <typename Handler>
      void do_wait(boost::tuple<Handler> handler)
      {
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            boost::asio::error::operation_not_supported
          )
        );
      } // do_wait

      boost::asio::io_service& io_service_;
      boost::asio::io_service& session_io_service_;
      boost::asio::io_service::strand strand_;      
      boost::asio::ip::tcp::acceptor acceptor_;
      bool started_;
      bool stopped_;      
      bool accept_in_progress_;      
      handler_allocator accept_allocator_;
      session_state_set sessions_;
    }; // class server

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_HPP
