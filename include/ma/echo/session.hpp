//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SESSION_HPP
#define MA_ECHO_SESSION_HPP

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <ma/echo/session_service.hpp>
#include <ma/echo/detail/session.hpp>

namespace ma
{    
  namespace echo
  {
    template <typename AsyncStream>
    class session : private boost::noncopyable 
    {
    private:
      typedef detail::session<AsyncStream> active_impl_type;

    public:
      typedef session_service<active_impl_type> service_type;      
      typedef typename service_type::implementation_type implementation_type;
      typedef typename service_type::next_layer_type next_layer_type;
      typedef typename service_type::lowest_layer_type lowest_layer_type;    

      explicit session(boost::asio::io_service& io_service)
        : service_(boost::asio::use_service<service_type>(io_service))
      {
        service_.construct(implementation_);
      }

      ~session()
      {
        service_.destroy(implementation_);
      }      

      boost::asio::io_service& io_service()
      {
        return service_.get_io_service();
      }

      boost::asio::io_service& get_io_service()
      {
        return service_.get_io_service();
      }

      next_layer_type& next_layer() const
      {
        return service_.next_layer(implementation_);
      }

      lowest_layer_type& lowest_layer() const
      {
        return service_.lowest_layer(implementation_);
      }

      template <typename Handler>
      void async_handshake(Handler handler)
      {
        service_.async_handshake(implementation_, handler);
      }

      template <typename Handler>
      void async_shutdown(Handler handler)
      {
        service_.async_shutdown(implementation_, handler);
      }

      template <typename Handler>
      void async_wait(Handler handler)
      {
        service_.async_wait(implementation_, handler);
      }
      
    private:
      // The service associated with the I/O object.
      service_type& service_;

      // The underlying implementation of the I/O object.
      implementation_type implementation_;

    }; // class session

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SESSION_HPP
