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
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/echo/server/session_config_fwd.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/io_service_set_fwd.hpp>
#include <ma/echo/server/session_manager_fwd.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      class session_manager 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session_manager>
      {
      private:
        typedef session_manager this_type;
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
          in_place_handler_allocator<128> start_wait_allocator_;
          in_place_handler_allocator<128> stop_allocator_;

          explicit session_proxy(boost::asio::io_service& io_service,
            const session_config& holded_session_config);
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
        explicit session_manager(io_service_set& io_services, 
          const session_manager_config& config);
        ~session_manager();        

        template <typename Handler>
        void async_start(Handler handler)
        {
          strand_.dispatch(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_start<Handler>, shared_from_this(), _1)));  
        } // async_start

        template <typename Handler>
        void async_stop(Handler handler)
        {
          strand_.dispatch(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_stop<Handler>, shared_from_this(), _1))); 
        } // async_stop

        template <typename Handler>
        void async_wait(Handler handler)
        {
          strand_.dispatch(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_wait<Handler>, shared_from_this(), _1)));  
        } // async_wait

      private:
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };

        template <typename Handler>
        void do_start(const Handler& handler)
        {
          boost::system::error_code error;
          start(error);
          io_service_.post(detail::bind_handler(handler, error));          
        } // do_start

        template <typename Handler>
        void do_stop(const Handler& handler)
        {
          boost::system::error_code error;
          bool completed;
          stop(error, completed);
          if (completed)
          {          
            io_service_.post(detail::bind_handler(handler, error));
          }
          else
          {
            stop_handler_.store(handler);
          }
        } // do_stop

        template <typename Handler>
        void do_wait(const Handler& handler)
        {
          boost::system::error_code error;
          bool completed;
          wait(error, completed);
          if (completed)
          {          
            io_service_.post(detail::bind_handler(handler, error));
          }
          else
          {          
            wait_handler_.store(handler);
          }  
        } // do_wait

        void start(boost::system::error_code& error);
        void stop(boost::system::error_code& error, bool& completed);
        void wait(boost::system::error_code& error, bool& completed);
        void accept_new_session();
        void handle_accept(const session_proxy_ptr& proxy, const boost::system::error_code& error);
        bool may_complete_stop() const;
        void complete_stop();
        void start_session(const session_proxy_ptr& proxy);
        void stop_session(const session_proxy_ptr& proxy);
        void wait_session(const session_proxy_ptr& proxy);
        static void dispatch_session_start(const session_manager_weak_ptr& weak_this_ptr,
          const session_proxy_ptr& proxy, const boost::system::error_code& error);
        void handle_session_start(const session_proxy_ptr& proxy, const boost::system::error_code& error);
        static void dispatch_session_wait(const session_manager_weak_ptr& weak_this_ptr,
          const session_proxy_ptr& proxy, const boost::system::error_code& error);
        void handle_session_wait(const session_proxy_ptr& proxy, const boost::system::error_code& error);        
        static void dispatch_session_stop(const session_manager_weak_ptr& weak_this_ptr,
          const session_proxy_ptr& proxy, const boost::system::error_code& error);
        void handle_session_stop(const session_proxy_ptr& proxy, const boost::system::error_code& error);        
        void recycle_session(const session_proxy_ptr& proxy);

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
        session_manager_config config_;        
        in_place_handler_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
