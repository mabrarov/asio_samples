//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
#define MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#include <cstddef>
#include <csignal>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/bind_handler.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/handler_ptr.hpp>
#include <ma/detail/service_base.hpp>
#include <ma/detail/intrusive_list.hpp>
#include <ma/detail/utility.hpp>

namespace ma {
namespace windows {

class console_signal_service_base
  : private detail::intrusive_list<console_signal_service_base>::base_hook
  , private boost::noncopyable
{
private:
  typedef console_signal_service_base this_type;
  friend class detail::intrusive_list<console_signal_service_base>;

public:
  virtual bool deliver_signal(int signal) = 0;

protected:
  class system_service;
  typedef detail::shared_ptr<system_service> system_service_ptr;

  console_signal_service_base()
  {
  }

  ~console_signal_service_base()
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }
}; // class console_signal_service_base

/// asio::io_service::service implementing console_signal.
class console_signal_service
  : public detail::service_base<console_signal_service>
  , public console_signal_service_base
{
private:
  class handler_base
    : public detail::intrusive_forward_list<handler_base>::base_hook
  {
  private:
    typedef handler_base this_type;
    typedef detail::intrusive_forward_list<handler_base>::base_hook base_type;

  public:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

    virtual void destroy() = 0;
    virtual void post(const boost::system::error_code&, int) = 0;

#else

    void destroy();
    void post(const boost::system::error_code&, int);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  protected:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

    handler_base();

#else

    typedef void (*destroy_func_type)(this_type*);
    typedef void (*post_func_type)(this_type*,
        const boost::system::error_code&, int);

    handler_base(destroy_func_type, post_func_type);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

    ~handler_base();
    handler_base(const this_type&);

    MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

  private:
#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
    destroy_func_type destroy_func_;
    post_func_type    post_func_;
#endif
  }; // class handler_base

  typedef detail::intrusive_forward_list<handler_base> handler_list;

  // Base class for implementation that helps to hide
  // public inheritance from detail::intrusive_list::base_hook
  class impl_base
    : public detail::intrusive_list<impl_base>::base_hook
    , private boost::noncopyable
  {
  public:
    impl_base();

#if !defined(NDEBUG)
    ~impl_base();
#endif

  private:
    friend class console_signal_service;
    handler_list handlers_;
  }; // class impl_base

public:
  class implementation_type : private impl_base
  {
  private:
    friend class console_signal_service;
  }; // class implementation_type

  explicit console_signal_service(boost::asio::io_service& io_service);
  void construct(implementation_type& impl);
  void destroy(implementation_type& impl);

  template <typename Handler>
  void async_wait(implementation_type& impl, Handler handler);

  std::size_t cancel(implementation_type& impl,
      boost::system::error_code& error);

  virtual bool deliver_signal(int signal);

protected:
  virtual ~console_signal_service();

private:
  typedef std::size_t queued_signals_counter;

  struct handler_list_owner;
  class handler_list_binder;

  template <typename Handler>
  class handler_wrapper;

  typedef detail::mutex                     mutex_type;
  typedef detail::lock_guard<mutex_type>    lock_guard;
  typedef detail::intrusive_list<impl_base> impl_base_list;

  virtual void shutdown_service();

  mutex_type mutex_;
  // Double-linked intrusive list of active implementations.
  impl_base_list impl_list_;
  queued_signals_counter sigint_queued_signals_;
  queued_signals_counter sigterm_queued_signals_;
  // Shutdown state flag.
  bool shutdown_;
  system_service_ptr system_service_;
}; // class console_signal_service

template <typename Handler>
class console_signal_service::handler_wrapper : public handler_base
{
private:
  typedef handler_wrapper<Handler> this_type;
  typedef handler_base             base_type;

public:

  template <typename H>
  handler_wrapper(boost::asio::io_service&, MA_FWD_REF(H));

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  handler_wrapper(this_type&&);
  handler_wrapper(const this_type&);

#endif

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  void destroy();
  void post(const boost::system::error_code&, int);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

#if !defined(NDEBUG)
  ~handler_wrapper();
#endif

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

private:
  static void do_destroy(base_type*);
  static void do_post(base_type*, const boost::system::error_code&, int);

  boost::asio::io_service::work work_;
  Handler handler_;
}; // class console_signal_service::handler_wrapper

template <typename Handler>
void console_signal_service::async_wait(implementation_type& impl,
    Handler handler)
{
  lock_guard lock(mutex_);
  if (shutdown_)
  {
    ma::get_io_context(*this).post(ma::bind_handler(detail::move(handler),
        boost::asio::error::operation_aborted, 0));
    return;
  }

  if (sigint_queued_signals_)
  {
    --sigint_queued_signals_;
    ma::get_io_context(*this).post(ma::bind_handler(detail::move(handler),
        boost::system::error_code(), SIGINT));
    return;
  }
  if (sigterm_queued_signals_)
  {
    --sigterm_queued_signals_;
    ma::get_io_context(*this).post(ma::bind_handler(detail::move(handler),
        boost::system::error_code(), SIGTERM));
    return;
  }

  typedef handler_wrapper<Handler> value_type;
  typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;

  detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
  detail::handler_ptr<alloc_traits> ptr(raw_ptr,
      detail::ref(ma::get_io_context(*this)), detail::move(handler));

  // Add handler to the list of waiting handlers.
  impl.handlers_.push_front(*ptr.get());
  ptr.release();
}

template <typename Handler>
template <typename H>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    boost::asio::io_service& io_service, MA_FWD_REF(H) handler)
#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  : base_type()
#else
  : base_type(&this_type::do_destroy, &this_type::do_post)
#endif
  , work_(io_service)
  , handler_(detail::forward<H>(handler))
{
}

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

template <typename Handler>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    this_type&& other)
  : base_type(detail::move(other))
  , work_(detail::move(other.work_))
  , handler_(detail::move(other.handler_))
{
}

template <typename Handler>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    const this_type& other)
  : base_type(other)
  , work_(other.work_)
  , handler_(other.handler_)
{
}

#endif

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::destroy()
{
  do_destroy(this);
}

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::post(
    const boost::system::error_code& error, int signal)
{
  do_post(this, error, signal);
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

#if !defined(NDEBUG)

template <typename Handler>
console_signal_service::handler_wrapper<Handler>::~handler_wrapper()
{
}

#endif // !defined(NDEBUG)

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::do_destroy(
    base_type* base)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  // Take ownership of the wrapper object
  // The deallocation of wrapper object will be done
  // throw the handler stored in wrapper
  typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
  detail::handler_ptr<alloc_traits> ptr(this_ptr->handler_, this_ptr);
  // Make a local copy of handler stored at wrapper object
  // This local copy will be used for wrapper's memory deallocation later
  Handler handler(detail::move(this_ptr->handler_));
  // Change the handler which will be used
  // for wrapper's memory deallocation
  ptr.set_alloc_context(handler);
  // Destroy wrapper object and deallocate its memory
  // throw the local copy of handler
  ptr.reset();
}

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::do_post(
    base_type* base, const boost::system::error_code& error, int signal)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  // Take ownership of the wrapper object
  // The deallocation of wrapper object will be done
  // throw the handler stored in wrapper
  typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
  detail::handler_ptr<alloc_traits> ptr(this_ptr->handler_, this_ptr);
  // Make a local copy of handler stored at wrapper object
  // This local copy will be used for wrapper's memory deallocation later
  Handler handler(detail::move(this_ptr->handler_));
  // Change the handler which will be used for wrapper's memory deallocation
  ptr.set_alloc_context(handler);
  // Make copies of other data placed at wrapper object
  // These copies will be used after the wrapper object destruction
  // and deallocation of its memory
  boost::asio::io_service::work work(detail::move(this_ptr->work_));
  // Destroy wrapper object and deallocate its memory
  // through the local copy of handler
  ptr.reset();
  // Post the copy of handler's local copy to io_service
  boost::asio::io_service& io_service = ma::get_io_context(work);
  io_service.post(ma::bind_handler(detail::move(handler), error, signal));
}

} // namespace windows
} // namespace ma

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#endif // MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
