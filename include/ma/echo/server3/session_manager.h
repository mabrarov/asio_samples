//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_SESSION_MANAGER_H
#define MA_ECHO_SERVER3_SESSION_MANAGER_H

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <ma/echo/server3/session.h>
#include <ma/echo/server3/session_proxy_fwd.h>
#include <ma/echo/server3/session_proxy_list.h>
#include <ma/echo/server3/session_manager_handler_fwd.h>
#include <ma/echo/server3/session_manager_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server3
    {    
      class session_manager 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session_manager>
      {
      private:
        typedef session_manager this_type;        

      public:
        struct config
        { 
          int listen_backlog_;
          std::size_t max_sessions_;
          std::size_t recycled_sessions_;
          boost::asio::ip::tcp::endpoint endpoint_;                    
          session::config session_config_;

          explicit config(const boost::asio::ip::tcp::endpoint& endpoint,
            std::size_t max_sessions,
            std::size_t recycled_sessions,
            int listen_backlog,
            const session::config& session_config);
        }; // struct config

        explicit session_manager(boost::asio::io_service& io_service,
          boost::asio::io_service& session_io_service,
          const config& config);
        ~session_manager();

        void async_start(const allocator_ptr& operation_allocator,
          const session_manager_start_handler_weak_ptr& handler);        
        void async_stop(const allocator_ptr& operation_allocator,
          const session_manager_stop_handler_weak_ptr& handler);        
        void async_wait(const allocator_ptr& operation_allocator,
          const session_manager_wait_handler_weak_ptr& handler);        

      private:    
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };
        
        typedef std::pair<session_manager_stop_handler_weak_ptr, allocator_ptr> stop_handler_storage;
        typedef std::pair<session_manager_wait_handler_weak_ptr, allocator_ptr> wait_handler_storage;

        friend class session_proxy;

        boost::asio::io_service::strand& strand();
        void do_start(const allocator_ptr& operation_allocator,
          const session_manager_start_handler_weak_ptr& handler);        
        void do_stop(const allocator_ptr& operation_allocator,
          const session_manager_stop_handler_weak_ptr& handler);        
        void do_wait(const allocator_ptr& operation_allocator,
          const session_manager_wait_handler_weak_ptr& handler);
        void accept_new_session();
        void handle_accept(const session_proxy_ptr& new_session_proxy,
          const boost::system::error_code& error);
        bool may_complete_stop() const;               

        void start_session(const session_proxy_ptr& accepted_session_proxy);
        void stop_session(const session_proxy_ptr& started_session_proxy);        
        void wait_session(const session_proxy_ptr& started_session_proxy);                        

        void handle_session_start(const session_proxy_ptr& started_session_proxy,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);        
        void handle_session_wait(const session_proxy_ptr& waited_session_proxy,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);                
        void handle_session_stop(const session_proxy_ptr& stopped_session_proxy,
          const allocator_ptr& operation_allocator,
          const boost::system::error_code& error);
        void recycle_session(const session_proxy_ptr& recycled_session_proxy);
        bool has_wait_handler() const;
        void invoke_wait_handler(const boost::system::error_code& error);
        void invoke_stop_handler(const boost::system::error_code& error);
        
        bool accept_in_progress_;
        state_type state_;
        std::size_t pending_operations_;                
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::io_service& session_io_service_;            
        stop_handler_storage stop_handler_;
        wait_handler_storage wait_handler_;                
        session_proxy_list active_session_proxies_;
        session_proxy_list recycled_session_proxies_;
        boost::system::error_code last_accept_error_;
        boost::system::error_code stop_error_;      
        config config_;        
        in_place_handler_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_SESSION_MANAGER_H
