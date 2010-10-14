//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_HPP
#define MA_ECHO_CLIENT1_SESSION_HPP

#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/echo/client1/session_config.hpp>
#include <ma/echo/client1/session_fwd.hpp>

namespace ma
{    
  namespace echo
  {
    namespace client1
    {      
      class session 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session>
      {
      private:
        typedef session this_type;        

      public:        
        explicit session(boost::asio::io_service& io_service, const session_config& config);

        ~session()
        {        
        } // ~session        

        boost::asio::ip::tcp::socket& session::socket()
        {
          return socket_;
        } // socket

        void reset();

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
            stop_handler_.put(handler);            
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
            wait_handler_.put(handler);
          } 
        } // do_wait

        void start(boost::system::error_code& error);        
        void stop(boost::system::error_code& error, bool& completed);
        void wait(boost::system::error_code& error, bool& completed);
        bool may_complete_stop() const;
        void complete_stop();        
        void read_some();        
        void write_some();        
        void handle_read_some(const boost::system::error_code& error, const std::size_t bytes_transferred);        
        void handle_write_some(const boost::system::error_code& error, const std::size_t bytes_transferred);        
        
        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        state_type state_;
        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::socket socket_;
        handler_storage<boost::system::error_code> wait_handler_;
        handler_storage<boost::system::error_code> stop_handler_;
        boost::system::error_code error_;
        boost::system::error_code stop_error_;
        session_config config_;        
        cyclic_buffer buffer_;
        in_place_handler_allocator<640> write_allocator_;
        in_place_handler_allocator<256> read_allocator_;
      }; // class session 

    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_HPP
