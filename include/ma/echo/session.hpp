//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SESSION_HPP
#define MA_ECHO_SESSION_HPP

#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/circular_buffer.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>

namespace ma
{    
  namespace echo
  {    
    class session;
    typedef boost::shared_ptr<session> session_ptr;

    class session 
      : private boost::noncopyable 
      , public boost::enable_shared_from_this<session>
    {
    private:
      typedef session this_type;

    public:
      explicit session(boost::asio::io_service& io_service)
        : io_service_(io_service)
        , strand_(io_service)
        , socket_(io_service)
      {        
      }

      ~session()
      {        
      }
      
      boost::asio::ip::tcp::socket& socket()
      {
        return socket_;
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
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            boost::get<0>(handler), 
            boost::asio::error::operation_not_supported
          )
        );
      }      

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
      }

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
      }

      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;      
      boost::asio::ip::tcp::socket socket_;
      bool started_;
      bool stopped_;      
      bool write_in_progress_;
      bool read_in_progress_;
      handler_allocator write_allocator_;
      handler_allocator read_allocator_;
    }; // class session

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SESSION_HPP
