//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/steady_deadline_timer.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace echo {
namespace server {

class session
  : private boost::noncopyable
  , public boost::enable_shared_from_this<session>
{
private:
  typedef session this_type;

public:
  typedef boost::asio::ip::tcp protocol_type;

  static session_ptr create(boost::asio::io_service& io_service,
      const session_config& config);

  protocol_type::socket& socket();

  void reset();

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(Handler&& handler);

  template <typename Handler>
  void async_stop(Handler&& handler);

  template <typename Handler>
  void async_wait(Handler&& handler);

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler);

  template <typename Handler>
  void async_stop(const Handler& handler);

  template <typename Handler>
  void async_wait(const Handler& handler);

#endif // defined(MA_HAS_RVALUE_REFS)

protected:
  session(boost::asio::io_service&, const session_config&);
  ~session();

private:

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) \
    && !(defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  // Home-grown binder to support move semantic
  template <typename Arg>
  class forward_handler_binder;

#endif

  struct extern_state
  {
    enum value_t {ready, work, stop, stopped};
  };

  struct intern_state
  {
    enum value_t {work, shutdown, stop, stopped};
  };

  struct read_state
  {
    enum value_t {wait, in_progress, stopped};
  };

  struct write_state
  {
    enum value_t {wait, in_progress, stopped};
  };

  struct timer_state
  {
    enum value_t {ready, in_progress, stopped};
  };

  typedef boost::optional<boost::system::error_code> optional_error_code;
  typedef steady_deadline_timer          deadline_timer;
  typedef deadline_timer::duration_type  duration_type;
  typedef boost::optional<duration_type> optional_duration;

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  template <typename Handler>
  void start_extern_start(const Handler&);

  template <typename Handler>
  void start_extern_stop(const Handler&);

  template <typename Handler>
  void start_extern_wait(const Handler&);

  void handle_read(const boost::system::error_code&, std::size_t);
  void handle_write(const boost::system::error_code&, std::size_t);
  void handle_timer(const boost::system::error_code&);

#endif // !(defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_HAS_LAMBDA) 
       //     && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  boost::system::error_code do_start_extern_start();
  optional_error_code do_start_extern_stop();
  optional_error_code do_start_extern_wait();
  void complete_extern_stop(const boost::system::error_code&);
  void complete_extern_wait(const boost::system::error_code&);

  void handle_read_at_work(const boost::system::error_code&, std::size_t);
  void handle_read_at_shutdown(const boost::system::error_code&, std::size_t);
  void handle_read_at_stop(const boost::system::error_code&, std::size_t);

  void handle_write_at_work(const boost::system::error_code&, std::size_t);
  void handle_write_at_shutdown(const boost::system::error_code&, std::size_t);
  void handle_write_at_stop(const boost::system::error_code&, std::size_t);

  void handle_timer_at_work(const boost::system::error_code&);
  void handle_timer_at_stop(const boost::system::error_code&);

  void continue_work();
  void continue_timer_wait();
  void continue_shutdown();
  void continue_shutdown_at_read_wait();
  void continue_shutdown_at_read_in_progress();
  void continue_shutdown_at_read_stopped();
  void continue_stop();

  void start_passive_shutdown();
  void start_active_shutdown();
  void start_shutdown(const boost::system::error_code&);
  void start_stop(boost::system::error_code);

  void start_socket_read(const cyclic_buffer::mutable_buffers_type&);
  void start_socket_write(const cyclic_buffer::const_buffers_type&);
  void start_timer_wait();
  boost::system::error_code cancel_timer_wait();
  boost::system::error_code shutdown_socket();
  boost::system::error_code close_socket();
  boost::system::error_code apply_socket_options();

  static optional_duration to_optional_duration(
      const session_config::optional_time_duration& duration);

  const std::size_t                   max_transfer_size_;
  const session_config::optional_int  socket_recv_buffer_size_;
  const session_config::optional_int  socket_send_buffer_size_;
  const session_config::optional_bool no_delay_;
  const optional_duration             inactivity_timeout_;

  extern_state::value_t extern_state_;
  intern_state::value_t intern_state_;
  read_state::value_t   read_state_;
  write_state::value_t  write_state_;
  timer_state::value_t  timer_state_;
  bool                  timer_wait_cancelled_;
  bool                  timer_turned_;
  std::size_t           pending_operations_;

  boost::asio::io_service&        io_service_;
  boost::asio::io_service::strand strand_;
  protocol_type::socket           socket_;
  deadline_timer                  timer_;
  cyclic_buffer                   buffer_;
  boost::system::error_code       extern_wait_error_;

  handler_storage<boost::system::error_code> extern_wait_handler_;
  handler_storage<boost::system::error_code> extern_stop_handler_;

  in_place_handler_allocator<640> write_allocator_;
  in_place_handler_allocator<256> read_allocator_;
  in_place_handler_allocator<256> timer_allocator_;
}; // class session

inline session::protocol_type::socket& session::socket()
{
  return socket_;
}

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) \
    && !(defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Arg>
class session::forward_handler_binder
{
private:
  typedef forward_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (session::*func_type)(const Arg&);

  template <typename SessionPtr>
  forward_handler_binder(func_type func, SessionPtr&& session);

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  forward_handler_binder(this_type&&);
  forward_handler_binder(const this_type&);

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  void operator()(const Arg& arg);

private:
  func_type   func_;
  session_ptr session_;
}; // class session::forward_handler_binder

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) 
       //     && !(defined(MA_HAS_LAMBDA) 
       //         && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Handler>
void session::async_start(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_ptr shared_this = shared_from_this();

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler), 
      [shared_this](const handler_type& handler)
  {
    boost::system::error_code result = shared_this->do_start_extern_start();
    shared_this->io_service_.post(detail::bind_handler(handler, result));
  }));

#else  // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  typedef void (this_type::*func_type)(const handler_type&);
  func_type func = &this_type::start_extern_start<handler_type>;

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      forward_handler_binder<handler_type>(func, shared_from_this())));

#else  // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      boost::bind(func, shared_from_this(), _1)));

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#endif // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)
}

template <typename Handler>
void session::async_stop(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_ptr shared_this = shared_from_this();

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler), 
      [shared_this](const handler_type& handler)
  {
    if (optional_error_code result = shared_this->do_start_extern_stop())
    {
      shared_this->io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      shared_this->extern_stop_handler_.store(handler);
    }
  }));

#else  // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  typedef void (this_type::*func_type)(const handler_type&);
  func_type func = &this_type::start_extern_stop<handler_type>;

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      forward_handler_binder<handler_type>(func, shared_from_this())));

#else  // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      boost::bind(func, shared_from_this(), _1)));

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#endif // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)
}

template <typename Handler>
void session::async_wait(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_ptr shared_this = shared_from_this();

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler), 
      [shared_this](const handler_type& handler)
  {
    if (optional_error_code result = shared_this->do_start_extern_wait())
    {
      shared_this->io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      shared_this->extern_wait_handler_.store(handler);
    }
  }));

#else  // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  typedef void (this_type::*func_type)(const handler_type&);
  func_type func = &this_type::start_extern_wait<handler_type>;

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      forward_handler_binder<handler_type>(func, shared_from_this())));

#else  // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      boost::bind(func, shared_from_this(), _1)));

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#endif // defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)
}

#else  // defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
void session::async_start(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_start<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

template <typename Handler>
void session::async_stop(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_stop<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

template <typename Handler>
void session::async_wait(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_wait<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

#endif // defined(MA_HAS_RVALUE_REFS)

inline session::~session()
{
}

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Handler>
void session::start_extern_start(const Handler& handler)
{
  boost::system::error_code result = do_start_extern_start();
  io_service_.post(detail::bind_handler(handler, result));
}

template <typename Handler>
void session::start_extern_stop(const Handler& handler)
{
  if (optional_error_code result = do_start_extern_stop())
  {
    io_service_.post(detail::bind_handler(handler, *result));
  }
  else
  {
    extern_stop_handler_.store(handler);
  }
}

template <typename Handler>
void session::start_extern_wait(const Handler& handler)
{
  if (optional_error_code result = do_start_extern_wait())
  {
    io_service_.post(detail::bind_handler(handler, *result));
  }
  else
  {
    extern_wait_handler_.store(handler);
  }
}

#endif // !(defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_HAS_LAMBDA) 
       //     && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) \
    && !(defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Arg>
template <typename SessionPtr>
session::forward_handler_binder<Arg>::forward_handler_binder(
    func_type func, SessionPtr&& session)
  : func_(func)
  , session_(std::forward<SessionPtr>(session))
{
}

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
session::forward_handler_binder<Arg>::forward_handler_binder(
    this_type&& other)
  : func_(other.func_)
  , session_(std::move(other.session_))
{
}

template <typename Arg>
session::forward_handler_binder<Arg>::forward_handler_binder(
    const this_type& other)
  : func_(other.func_)
  , session_(other.session_)
{
}

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
void session::forward_handler_binder<Arg>::operator()(const Arg& arg)
{
  ((*session_).*func_)(arg);
}

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
       //     && !(defined(MA_HAS_LAMBDA) 
       //         && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_HPP
