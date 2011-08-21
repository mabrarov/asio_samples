//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_HPP
#define MA_ECHO_SERVER_SESSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/echo/server/session_options.hpp>
#include <ma/echo/server/session_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace echo {

namespace server{

class session 
  : private boost::noncopyable
  , public boost::enable_shared_from_this<session>
{
private:
  typedef session this_type;        
        
public:
  typedef boost::asio::ip::tcp protocol_type;

  session(boost::asio::io_service& io_service, const session_options& options);

  ~session()
  {
  }

  protocol_type::socket& socket()
  {
    return socket_;
  }

  void reset();                
        
#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(
            &this_type::begin_start<handler_type>, shared_from_this())));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        forward_handler_binder<handler_type>(
            &this_type::begin_stop<handler_type>, shared_from_this()))); 
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        forward_handler_binder<handler_type>(
            &this_type::begin_wait<handler_type>, shared_from_this())));
  }

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),  
        boost::bind(&this_type::begin_start<handler_type>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_stop<handler_type>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_wait<handler_type>, 
            shared_from_this(), _1)));
  }

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_start<Handler>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_stop<Handler>, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_wait<Handler>, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)
        
private:

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  // Home-grown binder to support move semantic
  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;
    typedef void (session::*function_type)(const Arg&);

    template <typename SessionPtr>
    forward_handler_binder(function_type function, SessionPtr&& the_session)
      : function_(function)
      , session_(std::forward<SessionPtr>(the_session))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    forward_handler_binder(this_type&& other)
      : function_(other.function_)
      , session_(std::move(other.session_))
    {
    }    

    forward_handler_binder(const this_type& other)
      : function_(other.function_)
      , session_(other.session_)
    {
    }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    void operator()(const Arg& arg)
    {
      ((*session_).*function_)(arg);
    }

  private:
    function_type function_;
    session_ptr   session_;
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  struct external_state
  {
    enum value_t
    {
      ready,
      work,
      stop,
      stopped
    };
  }; // struct external_state

  struct general_state
  {
    enum value_t
    {
      work,
      shutdown,
      stop,
      stopped
    };
  }; // struct general_state

  struct read_state
  {
    enum value_t
    {
      wait,
      in_progress,      
      stopped
    };
  }; // struct read_state

  struct write_state
  {
    enum value_t
    {
      wait,
      in_progress,
      stopped
    };
  }; // struct write_state

  struct timer_state
  {
    enum value_t
    {
      ready,
      in_progress,
      stopped
    };
  }; // struct timer_state
  
  template <typename Handler>
  void begin_start(const Handler& handler)
  {
    boost::system::error_code result = start();
    io_service_.post(detail::bind_handler(handler, result));
  }

  template <typename Handler>
  void begin_stop(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result = stop())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      stop_handler_.put(handler);            
    }
  }

  template <typename Handler>
  void begin_wait(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result = wait())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    } 
    else
    {
      wait_handler_.put(handler);
    }
  }

  boost::system::error_code start();
  boost::optional<boost::system::error_code> stop();
  boost::optional<boost::system::error_code> wait();                       
                
  void handle_read(const boost::system::error_code&, std::size_t);
  void handle_read_at_work(const boost::system::error_code&, std::size_t);
  void handle_read_at_shutdown(const boost::system::error_code&, std::size_t);
  void handle_read_at_stop(const boost::system::error_code&, std::size_t);

  void handle_write(const boost::system::error_code&, std::size_t);
  void handle_write_at_work(const boost::system::error_code&, std::size_t);
  void handle_write_at_shutdown(const boost::system::error_code&, std::size_t);
  void handle_write_at_stop(const boost::system::error_code&, std::size_t);

  void handle_timer(const boost::system::error_code&);
  void handle_timer_at_work(const boost::system::error_code&);
  void handle_timer_at_shutdown(const boost::system::error_code&);
  void handle_timer_at_stop(const boost::system::error_code&);
    
  void continue_work();

  void continue_timer_activity();
  boost::system::error_code cancel_timer_activity();
  boost::system::error_code close_socket();

  void continue_shutdown();
  void continue_shutdown_at_read_wait();
  void continue_shutdown_at_read_in_progress();
  void continue_shutdown_at_read_stopped();

  void continue_stop();

  void begin_passive_shutdown();
  void begin_active_shutdown();
  void begin_general_stop(boost::system::error_code);

  void notify_work_completion(const boost::system::error_code&);  
    
  void begin_socket_read(const cyclic_buffer::mutable_buffers_type&);
  void begin_socket_write(const cyclic_buffer::const_buffers_type&);
  boost::system::error_code begin_timer_wait();
  boost::system::error_code apply_socket_options();

  static void copy_error(boost::system::error_code& dst, 
      const boost::system::error_code& src);
        
  const session_options::optional_int  socket_recv_buffer_size_;
  const session_options::optional_int  socket_send_buffer_size_;
  const session_options::optional_bool no_delay_;
  const session_options::optional_time_duration inactivity_timeout_;
  
  external_state::value_t   external_state_;
  general_state::value_t    general_state_;
  read_state::value_t       read_state_;
  write_state::value_t      write_state_;
  timer_state::value_t      timer_state_;  
  bool                      timer_cancelled_;
  std::size_t               pending_operations_;  
  boost::system::error_code wait_error_;

  boost::asio::io_service&        io_service_;
  boost::asio::io_service::strand strand_;
  protocol_type::socket           socket_;
  boost::asio::deadline_timer     timer_;
  cyclic_buffer                   buffer_;  

  handler_storage<boost::system::error_code> wait_handler_;
  handler_storage<boost::system::error_code> stop_handler_;

  in_place_handler_allocator<640> write_allocator_;
  in_place_handler_allocator<256> read_allocator_;
  in_place_handler_allocator<256> inactivity_allocator_;
}; // class session

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_HPP
