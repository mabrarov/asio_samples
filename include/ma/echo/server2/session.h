//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_H
#define MA_ECHO_SERVER2_SESSION_H

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <ma/handler_storage.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/echo/server2/session_completion.h>
#include <ma/echo/server2/session_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server2
    {
      class session 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session>
      {
      private:
        typedef session this_type;              
        
      public:
        struct config
        {              
          std::size_t buffer_size_;
          int socket_recv_buffer_size_;
          int socket_send_buffer_size_;
          bool no_delay_;

          explicit config(std::size_t buffer_size,
            int socket_recv_buffer_size,
            int socket_send_buffer_size,
            bool no_delay);
        }; // struct config

        explicit session(boost::asio::io_service& io_service, const config& config);          
        ~session();        
        void reset();                
        boost::asio::ip::tcp::socket& socket();                        
        void async_start(const session_completion::handler& handler);
        void async_stop(const session_completion::handler& handler);
        void async_wait(const session_completion::handler& handler);
        
      private: 
        enum state_type
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };  

        void do_start(const session_completion::handler& handler);                
        void do_stop(const session_completion::handler& handler);
        bool may_complete_stop() const;
        void complete_stop();        
        void do_wait(const session_completion::handler& handler);
        void read_some();
        void write_some();
        void handle_read_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred);
        void handle_write_some(const boost::system::error_code& error,
          const std::size_t bytes_transferred);

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::socket socket_;
        handler_storage<boost::system::error_code> wait_handler_;
        handler_storage<boost::system::error_code> stop_handler_;
        boost::system::error_code error_;
        boost::system::error_code stop_error_;
        config config_;
        state_type state_;
        bool socket_write_in_progress_;
        bool socket_read_in_progress_;
        cyclic_buffer buffer_;
        in_place_handler_allocator<640> write_allocator_;
        in_place_handler_allocator<256> read_allocator_;
      }; // class session

    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_H
