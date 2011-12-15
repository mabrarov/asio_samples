//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace detail {

/// Simplified double-linked intrusive list of boost::shared_ptr.
/**
 * Const time insertion of boost::shared_ptr.
 * Const time deletion of boost::shared_ptr (deletion by value).
 *
 * Requirements:
 * if value is rvalue of type Value then expression
 * static_cast&lt;sp_intrusive_list&lt;Value&gt;::base_hook&amp;&gt;(value)
 * must be well formed and accessible from sp_intrusive_list&lt;Value&gt;.
 */
template <typename Value>
class sp_intrusive_list : private boost::noncopyable
{
public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;
  typedef boost::weak_ptr<Value>   weak_pointer;
  typedef boost::shared_ptr<Value> shared_pointer;

  /// Required hook for items of the list.
  class base_hook : private boost::noncopyable
  {
  private:
    friend class sp_intrusive_list<value_type>;
    weak_pointer   prev_;
    shared_pointer next_;
  }; // class base_hook

  /// Never throws
  sp_intrusive_list()
    : size_(0)
  {
  }

  ~sp_intrusive_list()
  {
    clear();
  }

  /// Never throws
  shared_pointer front() const
  {
    return front_;
  }

  /// Never throws
  static shared_pointer prev(const shared_pointer& value)
  {
    BOOST_ASSERT(value);
    return get_hook(*value).prev_.lock();
  }

  /// Never throws
  static shared_pointer next(const shared_pointer& value)
  {
    BOOST_ASSERT(value);
    return get_hook(*value).next_;
  }

  /// Never throws
  void push_front(const shared_pointer& value)
  {
    BOOST_ASSERT(value);

    base_hook& value_hook = get_hook(*value);

    BOOST_ASSERT(!value_hook.prev_.lock() && !value_hook.next_);

    value_hook.next_ = front_;
    if (front_)
    {
      base_hook& front_hook = get_hook(*front_);
      front_hook.prev_ = value;
    }
    front_ = value;
    ++size_;
  }

  void erase(const shared_pointer& value)
  {
    BOOST_ASSERT(value);

    base_hook& value_hook = get_hook(*value);
    if (value == front_)
    {
      front_ = value_hook.next_;
    }
    const shared_pointer prev = value_hook.prev_.lock();
    if (prev)
    {
      base_hook& prev_hook = get_hook(*prev);
      prev_hook.next_ = value_hook.next_;
    }
    if (value_hook.next_)
    {
      base_hook& next_hook = get_hook(*value_hook.next_);
      next_hook.prev_ = value_hook.prev_;
    }
    value_hook.prev_.reset();
    value_hook.next_.reset();
    --size_;

    BOOST_ASSERT(!value_hook.prev_.lock() && !value_hook.next_);
  }

  void clear()
  {
    // We don't want to have recusrive calls of wrapped_session's destructor
    // because the deep of such recursion may be equal to the size of list.
    // The last can be too great for the stack.
    while (front_)
    {
      base_hook& front_hook = get_hook(*front_);
      const shared_pointer tmp = front_hook.next_;
      front_hook.prev_.reset();
      front_hook.next_.reset();
      front_ = tmp;
    }
    size_ = 0;
  }

  std::size_t size() const
  {
    return size_;
  }

  bool empty() const
  {
    return 0 == size_;
  }

private:
  static base_hook& get_hook(reference value)
  {
    return static_cast<base_hook&>(value);
  }

  std::size_t size_;
  shared_pointer front_;
}; // class sp_intrusive_list

} // namespace detail

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

  session_manager(boost::asio::io_service& io_service,
      boost::asio::io_service& session_io_service,
      const session_manager_config& config);

  ~session_manager()
  {
  }

  void reset(bool free_recycled_sessions = true);

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(func, shared_from_this(), _1)));
  }

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(handler,
        boost::bind(func, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:
  struct  session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;

  struct session_wrapper
    : public detail::sp_intrusive_list<session_wrapper>::base_hook
  {
    typedef protocol_type::endpoint endpoint_type;

    struct state_type
    {
      enum value_t {ready, start, work, stop, stopped};
    };

    endpoint_type       remote_endpoint;
    session_ptr         session;
    state_type::value_t state;
    std::size_t         pending_operations;
    in_place_handler_allocator<144> start_wait_allocator;
    in_place_handler_allocator<144> stop_allocator;

    session_wrapper(boost::asio::io_service& io_service,
        const session_config& config);

#if !defined(NDEBUG)
    ~session_wrapper()
    {
    }
#endif

    void reset();

    bool has_pending_operations() const
    {
      return 0 != pending_operations;
    }

    bool is_starting() const
    {
      return state_type::start == state;
    }

    bool is_stopping() const
    {
      return state_type::stop == state;
    }

    bool is_working() const
    {
      return state_type::work == state;
    }

    void handle_operation_completion()
    {
      --pending_operations;
    }

    void mark_as_stopped()
    {
      state = state_type::stopped;
    }

    void mark_as_working()
    {
      state = state_type::work;
    }

    void start_started()
    {
      state = state_type::start;
      ++pending_operations;
    }

    void stop_started()
    {
      state = state_type::stop;
      ++pending_operations;
    }

    void wait_started()
    {
      ++pending_operations;
    }
  }; // struct session_wrapper

  typedef detail::sp_intrusive_list<session_wrapper> session_list;

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  // Home-grown binders to support move semantic
  class accept_handler_binder;
  class session_dispatch_binder;
  class session_handler_binder;

  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;

    typedef void (session_manager::*func_type)(const Arg&);

    template <typename SessionManagerPtr>
    forward_handler_binder(func_type func, SessionManagerPtr&& session_manager)
      : func_(func)
      , session_manager_(std::forward<SessionManagerPtr>(session_manager))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

    forward_handler_binder(this_type&& other)
      : func_(other.func_)
      , session_manager_(std::move(other.session_manager_))
    {
    }

    forward_handler_binder(const this_type& other)
      : func_(other.func_)
      , session_manager_(other.session_manager_)
    {
    }

#endif

    void operator()(const Arg& arg)
    {
      ((*session_manager_).*func_)(arg);
    }

  private:
    func_type func_;
    session_manager_ptr session_manager_;
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

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

  template <typename Handler>
  void start_extern_start(const Handler& handler)
  {
    boost::system::error_code error = do_start_extern_start();
    io_service_.post(detail::bind_handler(handler, error));
  }

  template <typename Handler>
  void start_extern_stop(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result =
        do_start_extern_stop())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      extern_stop_handler_.reset(handler);
    }
  }

  template <typename Handler>
  void start_extern_wait(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result =
        do_start_extern_wait())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      extern_wait_handler_.reset(handler);
    }
  }

  boost::system::error_code                  do_start_extern_start();
  boost::optional<boost::system::error_code> do_start_extern_stop();
  boost::optional<boost::system::error_code> do_start_extern_wait();
  void complete_extern_stop(const boost::system::error_code&);
  void complete_extern_wait(const boost::system::error_code&);

  void continue_work();

  void handle_accept(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_accept_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_accept_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_start(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_start_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_start_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_wait(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_wait_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_wait_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  void handle_session_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_stop_at_work(const session_wrapper_ptr&,
      const boost::system::error_code&);
  void handle_session_stop_at_stop(const session_wrapper_ptr&,
      const boost::system::error_code&);

  bool is_out_of_work() const;
  void start_stop(const boost::system::error_code&);
  void continue_stop();

  void start_accept_session(const session_wrapper_ptr&);
  void start_session_start(const session_wrapper_ptr&);
  void start_session_stop(const session_wrapper_ptr&);
  void start_session_wait(const session_wrapper_ptr&);

  void recycle(const session_wrapper_ptr&);
  session_wrapper_ptr create_session(boost::system::error_code& error);

  boost::system::error_code open_acceptor();
  boost::system::error_code close_acceptor();

  static void dispatch_handle_session_start(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_wait(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_stop(const session_manager_weak_ptr&,
      const session_wrapper_ptr&, const boost::system::error_code&);

  static void open(protocol_type::acceptor& acceptor,
      const protocol_type::endpoint& endpoint, int backlog,
      boost::system::error_code& error);

  const protocol_type::endpoint accepting_endpoint_;
  const int                     listen_backlog_;
  const std::size_t             max_session_count_;
  const std::size_t             recycled_session_count_;
  const session_config          managed_session_config_;

  extern_state::value_t extern_state_;
  intern_state::value_t intern_state_;
  accept_state::value_t accept_state_;
  std::size_t           pending_operations_;

  boost::asio::io_service&        io_service_;
  boost::asio::io_service&        session_io_service_;
  boost::asio::io_service::strand strand_;
  protocol_type::acceptor         acceptor_;
  session_list                    active_sessions_;
  session_list                    recycled_sessions_;
  boost::system::error_code       extern_wait_error_;

  handler_storage<boost::system::error_code> extern_wait_handler_;
  handler_storage<boost::system::error_code> extern_stop_handler_;

  in_place_handler_allocator<512> accept_allocator_;
}; // class session_manager

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
