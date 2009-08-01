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
    class server 
      : private boost::noncopyable 
      , public boost::enable_shared_from_this<server>
    {
    private:
      typedef server this_type;      

      struct session_evironment_type : private boost::noncopyable
      {
        typedef boost::shared_ptr<session_evironment_type> pointer;
        typedef boost::weak_ptr<session_evironment_type> weak_pointer;

        pointer next_;
        weak_pointer prev_;
        session::pointer session_;
        bool stop_in_progress_;

        explicit session_evironment_type(boost::asio::io_service& io_service)
          : session_(new session(io_service))
          , stop_in_progress_(false)
        {
        }
      }; // session_evironment_type

      class session_evironment_set : private boost::noncopyable
      {
      public:
        explicit session_evironment_set()
        {
        }

        void insert(session_evironment_type::pointer session_evironment)
        {
          session_evironment->next_ = front_;
          session_evironment->prev_.reset();
          if (front_)
          {
            front_->prev_ = session_evironment;
          }
          front_ = session_evironment;
        }

        void erase(session_evironment_type::pointer session_evironment)
        {
          if (front_ == session_evironment)
          {
            front_ = front_->next_;
          }
          session_evironment_type::pointer prev = session_evironment->prev_.lock();
          if (prev)
          {
            prev->next_ = session_evironment->next_;
          }
          if (session_evironment->next_)
          {
            session_evironment->next_->prev_ = prev;
          }
          session_evironment->prev_.reset();
          session_evironment->next_.reset();
        }

      private:
        session_evironment_type::pointer front_;
      }; // session_evironment_set

    public:
      typedef boost::asio::ip::tcp::acceptor acceptor_type;
      typedef boost::asio::ip::tcp::endpoint endpoint_type;
      typedef boost::shared_ptr<this_type> pointer;
      
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

      boost::asio::io_service& io_service()
      {
        return io_service_;
      }

      boost::asio::io_service& get_io_service()
      {
        return io_service_;
      }      

      boost::asio::io_service& session_io_service()
      {
        return session_io_service_;
      }

      boost::asio::io_service& get_session_io_service()
      {
        return session_io_service_;
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
      void async_serve(Handler handler)
      {
        strand_.dispatch
        (
          ma::make_context_alloc_handler
          (
            handler, 
            boost::bind
            (
              &this_type::do_serve<Handler>,
              shared_from_this(),
              boost::make_tuple(handler)
            )
          )
        );  
      } // async_serve

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
      void do_serve(boost::tuple<Handler> handler)
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
      } // do_serve

      boost::asio::io_service& io_service_;
      boost::asio::io_service& session_io_service_;
      boost::asio::io_service::strand strand_;      
      acceptor_type acceptor_;
      bool started_;
      bool stopped_;      
      bool accept_in_progress_;      
      handler_allocator accept_allocator_;
      session_evironment_set session_evironments_;
    }; // class server

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_HPP
