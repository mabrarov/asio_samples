//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/sp_intrusive_list.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_factory_fwd.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager_stats.hpp>
#include <ma/echo/server/session_manager_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace echo {
namespace server {

class session_manager
  : private boost::noncopyable
  , public boost::enable_shared_from_this<session_manager>
{
private:
  typedef session_manager this_type;

public:
  typedef boost::asio::ip::tcp protocol_type;

  // Note that session_io_service has to outlive io_service
  static session_manager_ptr create(boost::asio::io_service& io_service,
      session_factory& managed_session_factory,
      const session_manager_config& config);

  void reset(bool free_recycled_sessions = true);

  session_manager_stats stats();

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
  // Note that session_io_service has to outlive io_service
  session_manager(boost::asio::io_service&, session_factory&,
      const session_manager_config&);
  ~session_manager();

private:
  class stats_collector : private boost::noncopyable
  {
  private:
    typedef boost::mutex mutex_type;
    typedef boost::lock_guard<mutex_type> lock_guard_type;

  public:
    stats_collector();

    session_manager_stats stats();
    void set_active_session_count(std::size_t);
    void set_recycled_session_count(std::size_t);
    void session_accepted(const boost::system::error_code&);
    void session_stopped(const boost::system::error_code&);
    void reset();

  private:
    mutex_type mutex_;
    session_manager_stats stats_;
  }; // class stats_collector

  class session_wrapper_base
    : public sp_intrusive_list<session_wrapper_base>::base_hook
  {
  }; // class session_wrapper_base

  class session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;
  typedef sp_intrusive_list<session_wrapper_base> session_list;

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) \
    && !(defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  // Home-grown binders to support move semantic
  class accept_handler_binder;
  class session_dispatch_binder;
  class session_handler_binder;

  template <typename Arg>
  class forward_handler_binder;

#endif

  struct extern_state
  {
    enum value_t {ready, work, stop, stopped};
  };

  struct intern_state
  {
    enum value_t {work, stop, stopped};
  };

  struct accept_state
  {
    enum value_t {ready, in_progress, stopped};
  };

  typedef boost::optional<boost::system::error_code> optional_error_code;

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  template <typename Handler>
  void start_extern_start(const Handler&);

  template <typename Handler>
  void start_extern_stop(const Handler&);

  template <typename Handler>
  void start_extern_wait(const Handler&);

  void handle_accept(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_start(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_wait(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

#endif // !(defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_HAS_LAMBDA)
       //     && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  boost::system::error_code do_start_extern_start();
  optional_error_code do_start_extern_stop();
  optional_error_code do_start_extern_wait();
  void complete_extern_stop(const boost::system::error_code&);
  void complete_extern_wait(const boost::system::error_code&);

  void continue_work();

  void handle_accept_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_accept_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_start_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_start_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_wait_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_wait_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_stop_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_stop_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void start_stop(const boost::system::error_code&);
  void continue_stop();

  session_wrapper_ptr start_active_session_stop(
      session_wrapper_ptr begin, std::size_t max_count);
  void schedule_active_session_stop();

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))
  void handle_scheduled_active_session_stop();
#endif

  void start_accept_session(const session_wrapper_ptr&);
  void start_session_start(const session_wrapper_ptr&);
  void start_session_stop(const session_wrapper_ptr&);
  void start_session_wait(const session_wrapper_ptr&);

  void recycle(const session_wrapper_ptr&);
  session_wrapper_ptr create_session(boost::system::error_code& error);

  void add_to_active(const session_wrapper_ptr&);
  void add_to_recycled(const session_wrapper_ptr&);
  void remove_from_active(const session_wrapper_ptr&);
  void remove_from_recycled(const session_wrapper_ptr&);

  boost::system::error_code open_acceptor();
  boost::system::error_code close_acceptor();

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

  static void dispatch_handle_session_start(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_wait(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_stop(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);

#endif

  static void open(protocol_type::acceptor& acceptor,
      const protocol_type::endpoint& endpoint, int backlog,
      boost::system::error_code& error);

  const protocol_type::endpoint accepting_endpoint_;
  const int                     listen_backlog_;
  const std::size_t             max_session_count_;
  const std::size_t             recycled_session_count_;
  const std::size_t             max_stopping_sessions_;
  const session_config          managed_session_config_;

  extern_state::value_t extern_state_;
  intern_state::value_t intern_state_;
  accept_state::value_t accept_state_;
  std::size_t           pending_operations_;

  boost::asio::io_service&        io_service_;
  session_factory&                session_factory_;
  boost::asio::io_service::strand strand_;
  protocol_type::acceptor         acceptor_;
  session_list                    active_sessions_;
  session_list                    recycled_sessions_;
  session_wrapper_ptr             stopping_sessions_end_;
  boost::system::error_code       accept_error_;
  boost::system::error_code       extern_wait_error_;
  stats_collector                 stats_collector_;

  handler_storage<boost::system::error_code> extern_wait_handler_;
  handler_storage<boost::system::error_code> extern_stop_handler_;

  in_place_handler_allocator<512> accept_allocator_;
  in_place_handler_allocator<256> session_stop_allocator_;
}; // class session_manager

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR) \
    && !(defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Arg>
class session_manager::forward_handler_binder
{
private:
  typedef forward_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (session_manager::*func_type)(const Arg&);

  template <typename SessionManagerPtr>
  forward_handler_binder(func_type func, SessionManagerPtr&& session_manager);

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  forward_handler_binder(this_type&&);
  forward_handler_binder(const this_type&);

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  void operator()(const Arg& arg);

private:
  func_type func_;
  session_manager_ptr session_manager_;
}; // class session_manager::forward_handler_binder

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
       //     && !(defined(MA_HAS_LAMBDA)
       //         && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Handler>
void session_manager::async_start(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_manager_ptr shared_this = shared_from_this();

  strand_.post(make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      [shared_this](const handler_type& handler)
  {
    boost::system::error_code error = shared_this->do_start_extern_start();
    shared_this->io_service_.post(detail::bind_handler(handler, error));
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
void session_manager::async_stop(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_manager_ptr shared_this = shared_from_this();

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
void session_manager::async_wait(Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;

#if defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  session_manager_ptr shared_this = shared_from_this();

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
void session_manager::async_start(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_start<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

template <typename Handler>
void session_manager::async_stop(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_stop<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

template <typename Handler>
void session_manager::async_wait(const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (this_type::*func_type)(const handler_type&);

  func_type func = &this_type::start_extern_wait<handler_type>;

  strand_.post(make_explicit_context_alloc_handler(handler,
      boost::bind(func, shared_from_this(), _1)));
}

#endif // defined(MA_HAS_RVALUE_REFS)

inline session_manager::~session_manager()
{
}

#if !(defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_HAS_LAMBDA) && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

template <typename Handler>
void session_manager::start_extern_start(const Handler& handler)
{
  boost::system::error_code error = do_start_extern_start();
  io_service_.post(detail::bind_handler(handler, error));
}

template <typename Handler>
void session_manager::start_extern_stop(const Handler& handler)
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
void session_manager::start_extern_wait(const Handler& handler)
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
template <typename SessionManagerPtr>
session_manager::forward_handler_binder<Arg>::forward_handler_binder(
    func_type func, SessionManagerPtr&& session_manager)
  : func_(func)
  , session_manager_(std::forward<SessionManagerPtr>(session_manager))
{
}

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
session_manager::forward_handler_binder<Arg>::forward_handler_binder(
    this_type&& other)
  : func_(other.func_)
  , session_manager_(std::move(other.session_manager_))
{
}

template <typename Arg>
session_manager::forward_handler_binder<Arg>::forward_handler_binder(
    const this_type& other)
  : func_(other.func_)
  , session_manager_(other.session_manager_)
{
}

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
void session_manager::forward_handler_binder<Arg>::operator()(const Arg& arg)
{
  ((*session_manager_).*func_)(arg);
}

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
       //     && !(defined(MA_HAS_LAMBDA)
       //         && !defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR))

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
