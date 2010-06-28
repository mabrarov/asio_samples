//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_MANAGER_H
#define MA_ECHO_SERVER2_SESSION_MANAGER_H

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/echo/server2/session.h>
#include <ma/echo/server2/session_proxy_fwd.h>
#include <ma/echo/server2/session_proxy_list.h>
#include <ma/echo/server2/session_manager_completion.h>
#include <ma/echo/server2/session_manager_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server2
    {    
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

      public:
        struct settings
        {      
          boost::asio::ip::tcp::endpoint endpoint_;
          std::size_t max_sessions_;
          std::size_t recycled_sessions_;
          int listen_backlog_;
          session::settings session_settings_;

          explicit settings(const boost::asio::ip::tcp::endpoint& endpoint,
            std::size_t max_sessions,
            std::size_t recycled_sessions,
            int listen_backlog,
            const session::settings& session_settings);
        }; // struct settings

        explicit session_manager(boost::asio::io_service& io_service,
          boost::asio::io_service& session_io_service,
          const settings& settings);
        ~session_manager();        
        void async_start(const session_manager_completion::handler& handler);        
        void async_stop(const session_manager_completion::handler& handler);        
        void async_wait(const session_manager_completion::handler& handler);
        
      private:        
        void do_start(const session_manager_completion::handler& handler);        
        void do_stop(const session_manager_completion::handler& handler);        
        void do_wait(const session_manager_completion::handler& handler);
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

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::io_service& session_io_service_;            
        ma::handler_storage<boost::system::error_code> wait_handler_;
        ma::handler_storage<boost::system::error_code> stop_handler_;
        session_proxy_list active_session_proxies_;
        session_proxy_list recycled_session_proxies_;
        boost::system::error_code last_accept_error_;
        boost::system::error_code stop_error_;      
        settings settings_;
        std::size_t pending_operations_;
        state_type state_;
        bool accept_in_progress_;
        in_place_handler_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_MANAGER_H
