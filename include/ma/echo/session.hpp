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
#include <boost/tuple/tuple.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>

namespace ma
{    
  namespace echo
  {
    template <typename AsyncStream>
    class session 
      : private boost::noncopyable 
      , public boost::enable_shared_from_this<session<AsyncStream> >
    {
    private:
      typedef session<AsyncStream> this_type;

    public:
      typedef AsyncStream next_layer_type;
      typedef typename AsyncStream::lowest_layer_type lowest_layer_type;    

      explicit session(boost::asio::io_service& io_service)
        : io_service_(io_service)
        , strand_(io_service)
        , stream_(io_service)
      {        
      }

      ~session()
      {        
      }      
      
      next_layer_type& next_layer() const
      {
        return stream_;
      }

      lowest_layer_type& lowest_layer() const
      {
        return stream_.lowest_layer();
      }

      template <typename Handler>
      void async_handshake(Handler handler)
      {
        strand_.dispatch
        (
          ma::make_context_alloc_handler
          (
            handler, 
            boost::bind
            (
              &this_type::start_handshake<Handler>,
              shared_from_this(),
              boost::make_tuple(handler)
            )
          )
        );  
      }

      template <typename Handler>
      void async_shutdown(Handler handler)
      {
        strand_.dispatch
        (
          ma::make_context_alloc_handler
          (
            handler, 
            boost::bind
            (
              &this_type::start_shutdown<Handler>,
              shared_from_this(),
              boost::make_tuple(handler)
            )
          )
        );  
      }

      template <typename Handler>
      void async_serve(Handler handler)
      {
        strand_.dispatch
        (
          ma::make_context_alloc_handler
          (
            handler, 
            boost::bind
            (
              &this_type::start_serve<Handler>,
              shared_from_this(),
              boost::make_tuple(handler)
            )
          )
        );  
      }
      
    private:
      template <typename Handler>
      void start_handshake(boost::tuple<Handler> handler)
      {
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            handler, 
            boost::asio::error::operation_not_supported
          )
        );
      }      

      template <typename Handler>
      void start_shutdown(boost::tuple<Handler> handler)
      {
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            handler, 
            boost::asio::error::operation_not_supported
          )
        );
      }

      template <typename Handler>
      void start_serve(boost::tuple<Handler> handler)
      {
        //todo
        io_service_.post
        (
          boost::asio::detail::bind_handler
          (
            handler, 
            boost::asio::error::operation_not_supported
          )
        );
      }

      boost::asio::io_service& io_service_;
      boost::asio::io_service::strand strand_;      
      next_layer_type stream_;
    }; // class session

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SESSION_HPP
