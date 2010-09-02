//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/echo/server/session.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      class session_manager;
      typedef boost::shared_ptr<session_manager> session_manager_ptr;    
      typedef boost::weak_ptr<session_manager>   session_manager_weak_ptr;    

      class session_manager 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session_manager>
      {
      private:
        typedef session_manager this_type;
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };
        struct session_proxy;
        typedef boost::shared_ptr<session_proxy> session_proxy_ptr;
        typedef boost::weak_ptr<session_proxy>   session_proxy_weak_ptr;      
        
        struct session_proxy : private boost::noncopyable
        {
          enum state_type
          {
            ready_to_start,
            start_in_progress,
            started,
            stop_in_progress,
            stopped
          };

          state_type state_;
          std::size_t pending_operations_;
          session_proxy_weak_ptr prev_;
          session_proxy_ptr next_;
          session_ptr session_;        
          boost::asio::ip::tcp::endpoint endpoint_;
          in_place_handler_allocator<256> start_wait_allocator_;
          in_place_handler_allocator<256> stop_allocator_;

          explicit session_proxy(boost::asio::io_service& io_service,
            const session::settings& session_settings);
          ~session_proxy();          
        }; // session_proxy

        class session_proxy_list : private boost::noncopyable
        {
        public:
          explicit session_proxy_list();

          void push_front(const session_proxy_ptr& value);
          void erase(const session_proxy_ptr& value);          
          std::size_t size() const;
          bool empty() const;
          session_proxy_ptr front() const;

        private:
          std::size_t size_;
          session_proxy_ptr front_;
        }; // session_proxy_list

      public:
        struct settings
        { 
          int listen_backlog_;
          std::size_t max_sessions_;
          std::size_t recycled_sessions_;
          boost::asio::ip::tcp::endpoint endpoint_;                    
          session::settings session_settings_;

          explicit settings(const boost::asio::ip::tcp::endpoint& endpoint,
            std::size_t max_sessions, std::size_t recycled_sessions,
            int listen_backlog, const session::settings& session_settings);
        }; // struct settings

        explicit session_manager(boost::asio::io_service& io_service,
          boost::asio::io_service& session_io_service, const settings& settings);
        ~session_manager();        

        template <typename Handler>
        void async_start(Handler handler)
        {
          strand_.dispatch
          (
            make_context_alloc_handler
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
            make_context_alloc_handler
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
            make_context_alloc_handler
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
            boost::system::error_code error;
            start_service(error);
            io_service_.post
            (
              detail::bind_handler
              (
                boost::get<0>(handler), 
                error
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
            stop_service();
            // Check for shutdown continuation
            if (may_complete_stop())
            {
              state_ = stopped;          
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
          else if (last_accept_error_ && active_session_proxies_.empty())
          {
            io_service_.post
            (
              detail::bind_handler
              (
                boost::get<0>(handler), 
                last_accept_error_
              )
            );
          }
          else if (wait_handler_.has_target())
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
            wait_handler_.store(boost::get<0>(handler));
          }  
        } // do_wait

        void start_service(boost::system::error_code& error);
        void stop_service();
        void accept_new_session();
        void handle_accept(const session_proxy_ptr& new_session_proxy,
          const boost::system::error_code& error);
        bool may_complete_stop() const;
        void start_session(const session_proxy_ptr& accepted_session_proxy);
        void stop_session(const session_proxy_ptr& started_session_proxy);
        void wait_session(const session_proxy_ptr& started_session_proxy);
        static void dispatch_session_start(const session_manager_weak_ptr& weak_session_manager,
          const session_proxy_ptr& started_session_proxy, const boost::system::error_code& error);
        void handle_session_start(const session_proxy_ptr& started_session_proxy,
          const boost::system::error_code& error);
        static void dispatch_session_wait(const session_manager_weak_ptr& weak_session_manager,
          const session_proxy_ptr& waited_session_proxy, const boost::system::error_code& error);
        void handle_session_wait(const session_proxy_ptr& waited_session_proxy,
          const boost::system::error_code& error);        
        static void dispatch_session_stop(const session_manager_weak_ptr& weak_session_manager,
          const session_proxy_ptr& stopped_session_proxy, const boost::system::error_code& error);
        void handle_session_stop(const session_proxy_ptr& stopped_session_proxy,
          const boost::system::error_code& error);        
        void recycle_session(const session_proxy_ptr& recycled_session_proxy);

        bool accept_in_progress_;
        state_type state_;
        std::size_t pending_operations_;
        boost::asio::io_service& io_service_;
        boost::asio::io_service& session_io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::acceptor acceptor_;        
        handler_storage<boost::system::error_code> wait_handler_;
        handler_storage<boost::system::error_code> stop_handler_;
        session_proxy_list active_session_proxies_;
        session_proxy_list recycled_session_proxies_;
        boost::system::error_code last_accept_error_;
        boost::system::error_code stop_error_;      
        settings settings_;        
        in_place_handler_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
