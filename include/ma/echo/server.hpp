//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_HPP
#define MA_ECHO_SERVER_HPP

#include <limits>
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
      struct wrapped_session;
      typedef boost::shared_ptr<wrapped_session> wrapped_session_ptr;
      typedef boost::weak_ptr<wrapped_session> wrapped_session_weak_ptr;

      struct wrapped_session : private boost::noncopyable
      {        
        wrapped_session_weak_ptr prev;
        wrapped_session_ptr next;
        session_ptr session;
        bool stop_in_progress;
        bool stopped;

        explicit wrapped_session(boost::asio::io_service& io_service)
          : session(new ma::echo::session(io_service))
          , stop_in_progress(false)
          , stopped(false)
        {
        }
      }; // wrapped_session

      class wrapped_session_list : private boost::noncopyable
      {
      public:
        explicit wrapped_session_list()
          : size_(0)
        {
        }

        void push_front(const wrapped_session_ptr& wrapped_session)
        {
          wrapped_session->next = front_;
          wrapped_session->prev.reset();
          if (front_)
          {
            front_->prev = wrapped_session;
          }
          front_ = wrapped_session;
          ++size_;
        }

        void erase(const wrapped_session_ptr& wrapped_session)
        {
          if (front_ == wrapped_session)
          {
            front_ = front_->next;
          }
          wrapped_session_ptr prev = wrapped_session->prev.lock();
          if (prev)
          {
            prev->next = wrapped_session->next;
          }
          if (wrapped_session->next)
          {
            wrapped_session->next->prev = prev;
          }
          wrapped_session->prev.reset();
          wrapped_session->next.reset();
          --size_;
        }

        std::size_t size() const
        {
          return size_;
        }

        wrapped_session_ptr front() const
        {
          return front_;
        }

      private:
        std::size_t size_;
        wrapped_session_ptr front_;
      }; // wrapped_session_list

    public:
      struct settings
      {      
        boost::asio::ip::tcp::endpoint endpoint;
        std::size_t max_sessions;
        int backlog;

        explicit settings(
          const boost::asio::ip::tcp::endpoint& listen_endpoint,
          std::size_t max_sessions_number = (std::numeric_limits<std::size_t>::max)(),
          int listen_backlog = 4)
          : endpoint(listen_endpoint)
          , max_sessions(max_sessions_number)
          , backlog(listen_backlog)
        {
        }
      }; // struct settings

      explicit server(boost::asio::io_service& io_service,
        boost::asio::io_service& session_io_service,
        const settings& settings)
        : io_service_(io_service)
        , session_io_service_(session_io_service)
        , strand_(io_service)
        , acceptor_(io_service)
        , settings_(settings)
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
        boost::system::error_code error;
        acceptor_.open(settings_.endpoint.protocol(), error);
        if (!error)
        {
          acceptor_.bind(settings_.endpoint, error);
          if (!error)
          {
            acceptor_.listen(settings_.backlog, error);
          }          
        }

        if (!error)
        {
          //todo
          error = boost::asio::error::operation_not_supported;
        }

        // For test only
        wrapped_session_ptr new_session(
          new wrapped_session(session_io_service_));
        wrapped_sessions_.push_front(new_session);
        wrapped_sessions_.erase(new_session);

        if (error)
        {
          boost::system::error_code ignored;
          acceptor_.close(ignored);
        }
        
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            error
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
      wrapped_session_list wrapped_sessions_;
      settings settings_;
    }; // class server

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_HPP
