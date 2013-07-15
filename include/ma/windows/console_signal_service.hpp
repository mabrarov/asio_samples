//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
#define MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/type_traits.hpp>
#include <ma/bind_handler.hpp>
#include <ma/detail/handler_ptr.hpp>
#include <ma/detail/service_base.hpp>
#include <ma/detail/intrusive_list.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace windows {

class console_signal_service_base 
  : public detail::intrusive_list<console_signal_service_base>::base_hook
{
public:
  virtual void handle_system_signal() = 0;
}; // class console_signal_service_base

/// asio::io_service::service implementing console_signal.
class console_signal_service
  : public detail::service_base<console_signal_service>
  , private console_signal_service_base
{
private:  
  class handler_base : public detail::intrusive_slist<handler_base>::base_hook
  {
  private:
    typedef handler_base this_type;
    typedef detail::intrusive_slist<handler_base>::base_hook base_type;

  public:

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

    virtual void destroy() = 0;
    virtual void post(const boost::system::error_code&) = 0;

#else

    void destroy();
    void post(const boost::system::error_code&);        

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

  protected:

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

    handler_base();

#else

    typedef void (*destroy_func_type)(this_type*);
    typedef void (*post_func_type)(this_type*, const boost::system::error_code&);

    handler_base(destroy_func_type, post_func_type);

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

    ~handler_base();
    handler_base(const this_type&);    

  private:
    this_type& operator=(const this_type&);

#if !defined(MA_TYPE_ERASURE_USE_VURTUAL)
    destroy_func_type destroy_func_;
    post_func_type    post_func_;
#endif
  }; // class handler_base

  typedef detail::intrusive_slist<handler_base> handler_list;

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

protected:
  virtual ~console_signal_service();

private:
  class handler_list_guard;
  class system_handler;
  class post_adapter;

  typedef boost::shared_ptr<system_handler> system_handler_ptr;

  template <typename Handler>
  class handler_wrapper;  

  typedef boost::mutex                      mutex_type;
  typedef boost::lock_guard<mutex_type>     lock_guard;
  typedef detail::intrusive_list<impl_base> impl_base_list;  

  virtual void shutdown_service();
  virtual void handle_system_signal();
  
  mutex_type mutex_;
  // Double-linked intrusive list of active implementations.
  impl_base_list impl_list_;
  // Shutdown state flag.
  bool shutdown_;
  system_handler_ptr system_handler_;
}; // class console_signal_service

template <typename Handler>
class console_signal_service::handler_wrapper : public handler_base
{
private:
  typedef handler_wrapper<Handler> this_type;
  typedef handler_base             base_type;

public:

#if defined(MA_HAS_RVALUE_REFS)

  template <typename H>
  handler_wrapper(boost::asio::io_service&, H&&);

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  handler_wrapper(this_type&&);
  handler_wrapper(const this_type&);

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

#else // defined(MA_HAS_RVALUE_REFS)

  handler_wrapper(boost::asio::io_service&, const Handler&);

#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)
  void destroy();
  void post(const boost::system::error_code&);
#endif

#if !defined(NDEBUG)
  ~handler_wrapper();
#endif

private:
  this_type& operator=(const this_type&);

  static void do_destroy(base_type*);
  static void do_post(base_type*, const boost::system::error_code&);

  boost::asio::io_service::work work_;
  Handler handler_;
}; // class console_signal_service::handler_wrapper

template <typename Handler>
void console_signal_service::async_wait(
    implementation_type& impl, Handler handler)
{
  lock_guard lock(mutex_);
  if (shutdown_)
  {
    get_io_service().post(
        bind_handler(handler, boost::asio::error::operation_aborted));
    return;
  }

  typedef handler_wrapper<Handler> value_type;
  typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;

  detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
#if defined(MA_HAS_RVALUE_REFS)
  detail::handler_ptr<alloc_traits> ptr(raw_ptr,
      boost::ref(this->get_io_service()), std::move(handler));
#else
  detail::handler_ptr<alloc_traits> ptr(raw_ptr,
      boost::ref(this->get_io_service()), handler);
#endif

  // Add handler to the list of waiting handlers.
  impl.handlers_.push_front(*ptr.get());
  ptr.release();
}

#if defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
template <typename H>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    boost::asio::io_service& io_service, H&& handler)
#if defined(MA_TYPE_ERASURE_USE_VURTUAL)
  : base_type()
#else
  : base_type(&this_type::do_destroy, &this_type::do_post)
#endif
  , work_(io_service)
  , handler_(std::forward<H>(handler))
{
}

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Handler>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    this_type&& other)
  : base_type(std::move(other))
  , work_(std::move(other.work_))
  , handler_(std::move(other.handler_))
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

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
console_signal_service::handler_wrapper<Handler>::handler_wrapper(
    boost::asio::io_service& io_service, const Handler& handler)
#if defined(MA_TYPE_ERASURE_USE_VURTUAL)
  : base_type()
#else
  : base_type(&this_type::do_destroy, &this_type::do_post)
#endif
  , work_(io_service)
  , handler_(handler)
{
}

#endif // defined(MA_HAS_RVALUE_REFS)

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::destroy()
{
  do_destroy(this);
}

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::post(
    const boost::system::error_code& error)
{
  do_post(this, error);
}

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

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
#if defined(MA_HAS_RVALUE_REFS)
  Handler handler(std::move(this_ptr->handler_));
#else
  Handler handler(this_ptr->handler_);
#endif
  // Change the handler which will be used
  // for wrapper's memory deallocation
  ptr.set_alloc_context(handler);
  // Destroy wrapper object and deallocate its memory
  // throw the local copy of handler
  ptr.reset();
}

template <typename Handler>
void console_signal_service::handler_wrapper<Handler>::do_post(
    base_type* base, const boost::system::error_code& error)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  // Take ownership of the wrapper object
  // The deallocation of wrapper object will be done
  // throw the handler stored in wrapper
  typedef detail::handler_alloc_traits<Handler, this_type> alloc_traits;
  detail::handler_ptr<alloc_traits> ptr(this_ptr->handler_, this_ptr);
  // Make a local copy of handler stored at wrapper object
  // This local copy will be used for wrapper's memory deallocation later
#if defined(MA_HAS_RVALUE_REFS)
  Handler handler(std::move(this_ptr->handler_));
#else
  Handler handler(this_ptr->handler_);
#endif
  // Change the handler which will be used for wrapper's memory deallocation
  ptr.set_alloc_context(handler);
  // Make copies of other data placed at wrapper object
  // These copies will be used after the wrapper object destruction
  // and deallocation of its memory
  boost::asio::io_service::work work(this_ptr->work_);
  // Destroy wrapper object and deallocate its memory
  // through the local copy of handler
  ptr.reset();
  // Post the copy of handler's local copy to io_service
  boost::asio::io_service& io_service = work.get_io_service();
#if defined(MA_HAS_RVALUE_REFS)
  io_service.post(ma::bind_handler(std::move(handler), error));
#else
  io_service.post(ma::bind_handler(handler, error));
#endif
}

} // namespace windows
} // namespace ma

#endif // MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
