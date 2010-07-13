//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_CLIENT1_SESSION_HPP
#define MA_ECHO_CLIENT1_SESSION_HPP

#include <utility>
#include <boost/utility.hpp>
#include <boost/system/error_code.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/client1/allocator.h>
#include <ma/echo/client1/session_handler_fwd.h>
#include <ma/echo/client1/session_fwd.h>

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
        struct settings
        {              
          std::size_t buffer_size_;
          int socket_recv_buffer_size_;
          int socket_send_buffer_size_;
          bool no_delay_;

          explicit settings(std::size_t buffer_size,
            int socket_recv_buffer_size,
            int socket_send_buffer_size,
            bool no_delay);
        }; // struct settings

        explicit session(boost::asio::io_service& io_service, const settings& settings);
        ~session();

        void reset();
        boost::asio::ip::tcp::socket& session::socket();
        void async_start(const allocator_ptr& operation_allocator,
          const session_start_handler_weak_ptr& handler);
        void async_stop(const allocator_ptr& operation_allocator,
          const session_stop_handler_weak_ptr& handler);
        void async_wait(const allocator_ptr& operation_allocator,
          const session_wait_handler_weak_ptr& handler);

      private:        
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        }; 
        
        typedef std::pair<session_stop_handler_weak_ptr, allocator_ptr> stop_handler_type;
        typedef std::pair<session_wait_handler_weak_ptr, allocator_ptr> wait_handler_type;
        
        void do_start(const allocator_ptr& operation_allocator,
          const session_start_handler_weak_ptr& handler);
        void do_wait(const allocator_ptr& operation_allocator,
          const session_wait_handler_weak_ptr& handler);
        void do_stop(const allocator_ptr& operation_allocator,
          const session_stop_handler_weak_ptr& handler);
        bool may_complete_stop() const;
        void complete_stop();        
        void read_some();
        void write_some();
        void handle_read_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred);
        void handle_write_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred);
        bool has_wait_handler() const;
        void invoke_wait_handler(const boost::system::error_code& error);
        void invoke_stop_handler(const boost::system::error_code& error);
        
        boost::asio::io_service::strand strand_;
        boost::asio::ip::tcp::socket socket_;
        stop_handler_type stop_handler_;
        wait_handler_type wait_handler_;                
        boost::system::error_code error_;
        boost::system::error_code stop_error_;
        settings settings_;
        state_type state_;
        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        cyclic_buffer buffer_;
        in_place_handler_allocator<640> write_allocator_;
        in_place_handler_allocator<256> read_allocator_;
      }; // class session 

    } // namespace client1
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_CLIENT1_SESSION_HPP
