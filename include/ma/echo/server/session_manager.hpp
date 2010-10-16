//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/echo/server/session_config_fwd.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_manager_config.hpp>
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
        struct  session_wrapper;
        typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;
        typedef boost::weak_ptr<session_wrapper>   session_wrapper_weak_ptr;      
        
        struct session_wrapper : private boost::noncopyable
        {
          enum state_type
          {
            ready_to_start,
            start_in_progress,
            started,
            stop_in_progress,
            stopped
          };

          state_type state;
          std::size_t pending_operations;
          session_wrapper_weak_ptr prev;
          session_wrapper_ptr next;
          session_ptr wrapped_session;        
          boost::asio::ip::tcp::endpoint remote_endpoint;
          in_place_handler_allocator<128> start_wait_allocator;
          in_place_handler_allocator<128> stop_allocator;

          explicit session_wrapper(boost::asio::io_service& io_service,
            const session_config& wrapped_session_config);

          ~session_wrapper()
          {
          } // ~session_wrapper

        }; // session_wrapper

        class session_wrapper_list : private boost::noncopyable
        {
        public:
          explicit session_wrapper_list();

          void push_front(const session_wrapper_ptr& value);
          void erase(const session_wrapper_ptr& value);          

          std::size_t size() const
          {
            return size_;
          } // size

          bool empty() const
          {
            return 0 == size_;
          } // empty

          session_wrapper_ptr front() const
          {
            return front_;
          } // front      

        private:
          std::size_t size_;
          session_wrapper_ptr front_;
        }; // session_wrapper_list

      public: 
        explicit session_manager(boost::asio::io_service& io_service, 
          boost::asio::io_service& session_io_service, 
          const session_manager_config& config);

        ~session_manager()
        {        
        } // ~session_manager  

        template <typename Handler>
        void async_start(Handler handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_start<Handler>, shared_from_this(), _1)));  
        } // async_start

        template <typename Handler>
        void async_stop(Handler handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
            boost::bind(&this_type::do_stop<Handler>, shared_from_this(), _1))); 
        } // async_stop

        template <typename Handler>
        void async_wait(Handler handler)
        {
          strand_.post(make_context_alloc_handler2(handler, 
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
          boost::system::error_code error = start();
          io_service_.post(detail::bind_handler(handler, error));          
        } // do_start

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
        } // do_stop

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
        } // do_wait

        boost::system::error_code start();
        boost::optional<boost::system::error_code> stop();
        boost::optional<boost::system::error_code> wait();
        void accept_new_session();
        void handle_accept(const session_wrapper_ptr& the_session, const boost::system::error_code& error);
        bool may_complete_stop() const;
        bool may_complete_wait() const;
        bool may_continue_accept() const;
        void complete_stop();
        void complete_wait();
        void start_session(const session_wrapper_ptr& the_session);
        void stop_session(const session_wrapper_ptr& the_session);
        void wait_session(const session_wrapper_ptr& the_session);
        static void dispatch_session_start(const session_manager_weak_ptr& this_weak_ptr,
          const session_wrapper_ptr& the_session, const boost::system::error_code& error);
        void handle_session_start(const session_wrapper_ptr& the_session, const boost::system::error_code& error);
        static void dispatch_session_wait(const session_manager_weak_ptr& this_weak_ptr,
          const session_wrapper_ptr& the_session, const boost::system::error_code& error);
        void handle_session_wait(const session_wrapper_ptr& the_session, const boost::system::error_code& error);        
        static void dispatch_session_stop(const session_manager_weak_ptr& this_weak_ptr,
          const session_wrapper_ptr& the_session, const boost::system::error_code& error);
        void handle_session_stop(const session_wrapper_ptr& the_session, const boost::system::error_code& error);        
        void recycle_session(const session_wrapper_ptr& the_session);

        bool accept_in_progress_;
        state_type state_;
        std::size_t pending_operations_;
        boost::asio::io_service& io_service_;
        boost::asio::io_service& session_io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::acceptor acceptor_;        
        handler_storage<boost::system::error_code> wait_handler_;
        handler_storage<boost::system::error_code> stop_handler_;
        session_wrapper_list active_sessions_;
        session_wrapper_list recycled_sessions_;
        boost::system::error_code wait_error_;
        boost::system::error_code stop_error_;      
        session_manager_config config_;        
        in_place_handler_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
