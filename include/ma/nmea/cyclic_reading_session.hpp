//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_NMEA_CYCLIC_READING_SESSION_HPP
#define MA_NMEA_CYCLIC_READING_SESSION_HPP

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <ma/nmea/session_service.hpp>
#include <ma/nmea/detail/cyclic_reading_session.hpp>

namespace ma
{  
  namespace nmea
  {
    template <typename AsyncStream>
    class cyclic_reading_session : private boost::noncopyable     
    {
    private:
      typedef detail::cyclic_reading_session<AsyncStream> active_impl_type;

    public:
      typedef session_service<active_impl_type> service_type;
      typedef typename active_impl_type::message_ptr message_ptr;
      typedef typename service_type::implementation_type implementation_type;
      typedef typename service_type::next_layer_type next_layer_type;
      typedef typename service_type::lowest_layer_type lowest_layer_type;    
      typedef typename active_impl_type::size_type size_type;
      typedef typename active_impl_type::read_capacity_type read_capacity_type;
      BOOST_STATIC_CONSTANT(size_type, min_read_buffer_size = active_impl_type::min_read_buffer_size);
      BOOST_STATIC_CONSTANT(read_capacity_type, min_read_message_buffer_capacity = active_impl_type::min_read_message_buffer_capacity);

      explicit cyclic_reading_session(
        boost::asio::io_service& io_service, 
        const size_type read_buffer_size = min_read_buffer_size,
        const read_capacity_type read_message_buffer_capacity = min_read_message_buffer_capacity,
        const std::string& frame_head = std::string("$"),
        const std::string& frame_tail = std::string("\x0a\x0d"))
        : service_(boost::asio::use_service<service_type>(io_service))
      {
        service_.construct(implementation_, 
          read_buffer_size, read_message_buffer_capacity,
          frame_head, frame_tail);
      }

      ~cyclic_reading_session()
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
      void async_write(const message_ptr& message, Handler handler)
      {
        service_.async_write(implementation_, message, handler);
      }

      template <typename Handler>
      void async_read(message_ptr& message, Handler handler)
      {
        service_.async_read(implementation_, message, handler);
      }

    private:
      // The service associated with the I/O object.
      service_type& service_;

      // The underlying implementation of the I/O object.
      implementation_type implementation_;
      
    }; // class cyclic_reading_session

  } // namespace nmea
} // namespace ma

#endif // MA_NMEA_CYCLIC_READING_SESSION_HPP