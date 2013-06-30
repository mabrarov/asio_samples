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
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/system/error_code.hpp>
#include <boost/utility/addressof.hpp>
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

/// asio::io_service::service implementing console_signal.
class console_signal_service
  : public detail::service_base<console_signal_service>
{
private:  
  class stored_base : public detail::intrusive_slist<stored_base>::base_hook
  {
  private:
    typedef stored_base this_type;
    typedef detail::intrusive_slist<stored_base>::base_hook base_type;

  public:

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

    stored_base();
    virtual void destroy() = 0;

#else

    typedef void (*destroy_func_type)(this_type*);

    stored_base(destroy_func_type);
    void destroy();

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

  protected:
    ~stored_base();
    stored_base(const this_type&);

  private:
    this_type& operator=(const this_type&);

#if !defined(MA_TYPE_ERASURE_USE_VURTUAL)
    destroy_func_type destroy_func_;
#endif
  }; // class stored_base

  typedef detail::intrusive_slist<stored_base> handler_list;

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

  std::size_t cancel(const implementation_type& impl, 
      boost::system::error_code& error);

protected:
  virtual ~console_signal_service();

private:
  class owning_handler_list;
  class handler_base;  

  template <typename Handler>
  class handler_wrapper;

  typedef boost::mutex                      mutex_type;
  typedef boost::lock_guard<mutex_type>     lock_guard;
  typedef detail::intrusive_list<impl_base> impl_base_list;

  virtual void shutdown_service();

  // Guard for the impl_list_
  mutex_type impl_list_mutex_;
  // Double-linked intrusive list of active implementations.
  impl_base_list impl_list_;
  // Shutdown state flag.
  bool shutdown_;
}; // class console_signal_service

class console_signal_service::owning_handler_list : public handler_list
{
public:
  ~owning_handler_list();
}; // class console_signal_service::owning_handler_list

class console_signal_service::handler_base : public stored_base
{
private:
  typedef handler_base this_type;
  typedef stored_base base_type;

public:

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

  handler_base();
  virtual void post(const boost::system::error_code&) = 0;

#else

  typedef void (*post_func_type)(this_type*, const boost::system::error_code&);

  handler_base(destroy_func_type, post_func_type);
  void post(const boost::system::error_code&);

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

protected:
  ~handler_base();
  handler_base(const this_type&);

private:
  this_type& operator=(const this_type&);

#if !defined(MA_TYPE_ERASURE_USE_VURTUAL)
  post_func_type   post_func_;
#endif
}; // class console_signal_service::handler_base

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

  static void do_destroy(stored_base*);
  static void do_post(base_type*, const boost::system::error_code&);

  boost::asio::io_service::work work_;
  Handler handler_;
}; // class console_signal_service::handler_wrapper

console_signal_service::owning_handler_list::~owning_handler_list()
{
  for (stored_base* stored = this->front(); stored; )
  {
    stored_base* next = this->next(*stored);
    stored->destroy();
    stored = next;
  }
}

inline console_signal_service::console_signal_service(
    boost::asio::io_service& io_service)
  : detail::service_base<console_signal_service>(io_service)  
{
  //todo
}

inline void console_signal_service::construct(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }

  // Add implementation to the list of active implementations.
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.push_front(impl);
  }
}

inline void console_signal_service::destroy(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }
    
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    // Take ownership of waiting handlers
    handlers.push_front_reversed(impl.handlers_);
    // Remove implementation from the list of active implementations.
    impl_list_.erase(impl);
  }

  // Cancel all waiting handlers.
  while (stored_base* handler = handlers.front())
  {
    handlers.pop_front();
    static_cast<handler_base*>(handler)->post(
        boost::asio::error::operation_aborted);
  }  
}

template <typename Handler>
inline void console_signal_service::async_wait(
    implementation_type& impl, Handler handler)
{
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
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    handlers.push_front(*ptr.get());    
    ptr.release();
  }
  
}

inline std::size_t console_signal_service::cancel(
    const implementation_type& impl, boost::system::error_code& error)
{
  // Take ownership of waiting handlers
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    handlers.push_front_reversed(impl.handlers_);
  }

  // Post all handlers to signal operation was aborted  
  std::size_t handler_count = 0;
  while (stored_base* handler = handlers.front())
  {
    handlers.pop_front();
    static_cast<handler_base*>(handler)->post(
        boost::asio::error::operation_aborted);
    ++handler_count;
  }
  
  error = boost::system::error_code();
  return handler_count;
}

inline void console_signal_service::shutdown_service()
{
  // Restrict usage of service.
  shutdown_ = true;  
  // Take ownership of all still active handlers.
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {      
      handlers.push_front_reversed(impl->handlers_);
    }
  }
}

} // namespace windows
} // namespace ma

#endif // MA_WINDOWS_CONSOLE_SIGNAL_SERVICE_HPP
