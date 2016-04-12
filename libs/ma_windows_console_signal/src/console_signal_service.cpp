//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/config.hpp>

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)

#include <windows.h>
#include <limits>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <ma/config.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/windows/console_signal_service.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/sp_singleton.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace windows {

class console_signal_service_base::system_service : private boost::noncopyable
{
private:
  typedef system_service this_type;

public:
  static system_service_ptr get_instance();
  void add_subscriber(console_signal_service_base&);
  void remove_subscriber(console_signal_service_base&);

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  system_service(const instance_guard_type&);
  ~system_service();

private:
  struct factory;

  typedef detail::mutex                  mutex_type;
  typedef detail::lock_guard<mutex_type> lock_guard;
  typedef detail::intrusive_list<console_signal_service_base> subscriber_list;

  static system_service_ptr get_nullable_instance();
  static BOOL WINAPI windows_ctrl_handler(DWORD);
  bool deliver_signal();
  
  instance_guard_type singleton_instance_guard_;
  mutex_type          subscribers_mutex_;
  subscriber_list     subscribers_;
}; // class console_signal_service_base::system_service

struct console_signal_service_base::system_service::factory
{
  system_service_ptr operator()(const instance_guard_type&);
}; // class console_signal_service_base::system_service::factory

struct console_signal_service::handler_list_owner : private boost::noncopyable
{
public:
  ~handler_list_owner();

  handler_list list;
}; // class console_signal_service::handler_list_owner

class console_signal_service::handler_list_binder
{
private:
  typedef handler_list_binder this_type;

public:
  typedef detail::shared_ptr<handler_list_owner> handler_list_owner_ptr;

  handler_list_binder(const handler_list_owner_ptr&);

#if defined(MA_HAS_RVALUE_REFS)
  handler_list_binder(const this_type&);
  handler_list_binder(this_type&&);
#endif // defined(MA_HAS_RVALUE_REFS)

  void operator()();

private:
  this_type& operator=(const this_type&);

  handler_list_owner_ptr handlers_;
}; // class console_signal_service::handler_list_binder

console_signal_service_base::system_service_ptr
console_signal_service_base::system_service::get_instance()
{
  return detail::sp_singleton<this_type>::get_instance(factory());
}

console_signal_service_base::system_service_ptr 
console_signal_service_base::system_service::get_nullable_instance()
{
  return detail::sp_singleton<this_type>::get_nullable_instance();
}

void console_signal_service_base::system_service::add_subscriber(
    console_signal_service_base& subscriber)
{
  lock_guard services_guard(subscribers_mutex_);
  subscribers_.push_back(subscriber);
}

void console_signal_service_base::system_service::remove_subscriber(
    console_signal_service_base& subscriber)
{
  lock_guard services_guard(subscribers_mutex_);
  subscribers_.erase(subscriber);
}

console_signal_service_base::system_service::system_service(
    const instance_guard_type& singleton_instance_guard)
  : singleton_instance_guard_(singleton_instance_guard)
{
  if (!::SetConsoleCtrlHandler(&this_type::windows_ctrl_handler, TRUE))
  {
    boost::system::error_code ec(
        ::GetLastError(), boost::system::system_category());
    boost::throw_exception(boost::system::system_error(ec));
  }
}

console_signal_service_base::system_service::~system_service()
{
  ::SetConsoleCtrlHandler(&this_type::windows_ctrl_handler, FALSE);
}

BOOL WINAPI console_signal_service_base::system_service::windows_ctrl_handler(
    DWORD ctrl_type)
{
  switch (ctrl_type)
  {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    if (system_service_ptr instance = get_nullable_instance())
    {
      if (instance->deliver_signal())
      {
        return TRUE;
      }
    }
  default:
    return FALSE;
  };
}

bool console_signal_service_base::system_service::deliver_signal()
{
  bool handled = false;
  lock_guard services_guard(subscribers_mutex_);
  for (console_signal_service_base* subscriber = subscribers_.front(); 
      subscriber; subscriber = subscribers_.next(*subscriber))
  {
    handled |= subscriber->deliver_signal();
  }
  return handled;
}

console_signal_service_base::system_service_ptr 
console_signal_service_base::system_service::factory::operator()(
    const instance_guard_type& singleton_instance_guard)
{
  typedef ma::shared_ptr_factory_helper<system_service> helper;  
  return detail::make_shared<helper>(singleton_instance_guard);
}

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

console_signal_service::handler_base::handler_base()
{
}

#else

void console_signal_service::handler_base::destroy()
{
  destroy_func_(this);
}

void console_signal_service::handler_base::post(
    const boost::system::error_code& error)
{
  post_func_(this, error);
}

console_signal_service::handler_base::handler_base(
    destroy_func_type destroy_func, post_func_type post_func)
  : destroy_func_(destroy_func)
  , post_func_(post_func)
{
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

console_signal_service::handler_base::~handler_base()
{
}

console_signal_service::handler_base::handler_base(const this_type& other)
  : base_type(other)
#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  , destroy_func_(other.destroy_func_)
  , post_func_(other.post_func_)
#endif
{
}

console_signal_service::impl_base::impl_base()
{
}

#if !defined(NDEBUG)

console_signal_service::impl_base::~impl_base()
{
}

#endif // !defined(NDEBUG)

console_signal_service::handler_list_owner::~handler_list_owner()
{
  for (handler_base* handler = list.front(); handler; )
  {
    handler_base* next = list.next(*handler);
    handler->destroy();
    handler = next;
  }
}

console_signal_service::handler_list_binder::handler_list_binder(
    const handler_list_owner_ptr& handlers)
  : handlers_(handlers)
{
}

#if defined(MA_HAS_RVALUE_REFS)

console_signal_service::handler_list_binder::handler_list_binder(
    const this_type& other)
  : handlers_(other.handlers_)
{
}

console_signal_service::handler_list_binder::handler_list_binder(
    this_type&& other)
  : handlers_(detail::move(other.handlers_))
{
}

#endif // defined(MA_HAS_RVALUE_REFS)

void console_signal_service::handler_list_binder::operator()()
{
  while (handler_base* handler = handlers_->list.front())
  {
    handlers_->list.pop_front();
    handler->post(boost::system::error_code());
  }
}

console_signal_service::console_signal_service(
    boost::asio::io_service& io_service)
  : detail::service_base<console_signal_service>(io_service)  
  , queued_signals_(0)
  , shutdown_(false)
  , system_service_(system_service::get_instance())
{
  system_service_->add_subscriber(*this);
}

console_signal_service::~console_signal_service()  
{
  system_service_->remove_subscriber(*this);
}

void console_signal_service::construct(implementation_type& impl)
{
  lock_guard lock(mutex_);
  if (!shutdown_)
  {
    impl_list_.push_back(impl);
  }
}

void console_signal_service::destroy(implementation_type& impl)
{  
  handler_list_owner handlers;
  {
    lock_guard lock(mutex_);
    if (!shutdown_)
    {
      // Take ownership of waiting handlers
#if defined(MA_HAS_RVALUE_REFS)
      handlers.list = detail::move(impl.handlers_);
#else
      handlers.list.swap(impl.handlers_);
#endif
      // Remove implementation from the list of active implementations.
      impl_list_.erase(impl);
    }
  }
  // Cancel all waiting handlers.
  while (handler_base* handler = handlers.list.front())
  {
    handlers.list.pop_front();
    handler->post(boost::asio::error::operation_aborted);
  }  
}

std::size_t console_signal_service::cancel(implementation_type& impl,
    boost::system::error_code& error)
{
  handler_list_owner handlers;
  {
    lock_guard lock(mutex_);
    if (!shutdown_)
    {
      // Take ownership of waiting handlers
#if defined(MA_HAS_RVALUE_REFS)
      handlers.list = detail::move(impl.handlers_);
#else
      handlers.list.swap(impl.handlers_);
#endif
    }
  }
  // Post all handlers to signal operation was aborted
  std::size_t handler_count = 0;
  while (handler_base* handler = handlers.list.front())
  {
    handlers.list.pop_front();
    handler->post(boost::asio::error::operation_aborted);
    ++handler_count;
  }
  error = boost::system::error_code();
  return handler_count;
}

void console_signal_service::shutdown_service()
{
  handler_list_owner handlers;
  {
    lock_guard lock(mutex_);
    // Restrict usage of service.
    shutdown_ = true;  
    // Take ownership of all still active handlers.
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {
      handlers.list.insert_front(impl->handlers_);
    }
  }
}

bool console_signal_service::deliver_signal()
{
  handler_list_binder::handler_list_owner_ptr handlers =
      detail::make_shared<handler_list_owner>();
  lock_guard lock(mutex_);
  if (impl_list_.empty())
  {
    return false;
  }
  if (!shutdown_)
  {
    // Take ownership of all still active handlers.
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {      
      handlers->list.insert_front(impl->handlers_);
    }
    if (handlers->list.empty())
    {
      if ((std::numeric_limits<queued_signals_counter>::max)() 
          != queued_signals_)
      {
        ++queued_signals_;
      }
    }
    else
    {
      get_io_service().post(handler_list_binder(handlers));
    }
  }
  return true;
}

} // namespace windows
} // namespace ma

#endif // defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
