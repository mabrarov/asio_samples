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

      struct session_fsm : private boost::noncopyable
      {
        typedef boost::shared_ptr<session_fsm> pointer;
        typedef boost::weak_ptr<session_fsm> weak_pointer;

        pointer next_;
        weak_pointer prev_;
        session::pointer session_;
        bool stop_in_progress_;

        explicit session_fsm(boost::asio::io_service& io_service)
          : session_(new session(io_service))
          , stop_in_progress_(false)
        {
        }
      }; // session_fsm

      class session_fsm_set : private boost::noncopyable
      {
      public:
        explicit session_fsm_set()
        {
        }

        void insert(session_fsm::pointer session_fsm)
        {
          session_fsm->next_ = front_;
          session_fsm->prev_.reset();
          if (front_)
          {
            front_->prev_ = session_fsm;
          }
          front_ = session_fsm;
        }

        void erase(session_fsm::pointer session_fsm)
        {
          if (front_ == session_fsm)
          {
            front_ = front_->next_;
          }
          session_fsm::pointer prev = session_fsm->prev_.lock();
          if (prev)
          {
            prev->next_ = session_fsm->next_;
          }
          if (session_fsm->next_)
          {
            session_fsm->next_->prev_ = prev;
          }
          session_fsm->prev_.reset();
          session_fsm->next_.reset();
        }

      private:
        session_fsm::pointer front_;
      }; // session_fsm_set

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

        session_fsm::pointer session_fsm(
          new session_fsm(session_io_service_));

        session_fsm_set_.insert(session_fsm);

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
      session_fsm_set session_fsm_set_;
    }; // class server

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_HPP
