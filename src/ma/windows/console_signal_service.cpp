//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <windows.h>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <ma/config.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/detail/sp_singleton.hpp>
#include <ma/windows/console_signal_service.hpp>

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

  typedef boost::mutex                  mutex_type;
  typedef boost::lock_guard<mutex_type> lock_guard;
  typedef detail::intrusive_list<console_signal_service_base> subscriber_list;

  static system_service_ptr get_nullable_instance();
  static BOOL WINAPI windows_ctrl_handler(DWORD);
  void deliver_signal();
  
  instance_guard_type singleton_instance_guard_;
  mutex_type          subscribers_mutex_;
  subscriber_list     subscribers_;
}; // class console_signal_service_base::system_service

struct console_signal_service_base::system_service::factory
{
  system_service_ptr operator()(const instance_guard_type&);
}; // class console_signal_service_base::system_service::factory

struct console_signal_service::handler_list_guard : private boost::noncopyable
{
public:
  ~handler_list_guard();

  handler_list list;
}; // class console_signal_service::handler_list_guard

class console_signal_service::post_adapter
{
private:
  typedef post_adapter this_type;

public:
  typedef boost::shared_ptr<handler_list_guard> handler_list_guard_ptr;

  post_adapter(const handler_list_guard_ptr&);

#if defined(MA_HAS_RVALUE_REFS)
  post_adapter(const this_type&);
  post_adapter(this_type&&);
#endif // defined(MA_HAS_RVALUE_REFS)

  void operator()();

private:
  this_type& operator=(const this_type&);

  handler_list_guard_ptr handlers_;
}; // class console_signal_service::post_adapter

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
      instance->deliver_signal();
      return TRUE;
    }
  default:
    return FALSE;
  };
}

void console_signal_service_base::system_service::deliver_signal()
{
  lock_guard services_guard(subscribers_mutex_);
  for (console_signal_service_base* subscriber = subscribers_.front(); 
      subscriber; subscriber = subscribers_.next(*subscriber))
  {
    subscriber->deliver_signal();
  }
}

console_signal_service_base::system_service_ptr 
console_signal_service_base::system_service::factory::operator()(
    const instance_guard_type& singleton_instance_guard)
{
  typedef ma::shared_ptr_factory_helper<system_service> helper;  
  return boost::make_shared<helper>(singleton_instance_guard);
}

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

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

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

console_signal_service::handler_base::~handler_base()
{
}

console_signal_service::handler_base::handler_base(const this_type& other)
  : base_type(other)
#if !defined(MA_TYPE_ERASURE_USE_VURTUAL)
  , destroy_func_(other.destroy_func_)
  , post_func_(other.post_func_)
#endif
{
}

console_signal_service::handler_list_guard::~handler_list_guard()
{
  for (handler_base* handler = list.front(); handler; )
  {
    handler_base* next = list.next(*handler);
    handler->destroy();
    handler = next;
  }
}

console_signal_service::post_adapter::post_adapter(
    const handler_list_guard_ptr& handlers)
  : handlers_(handlers)
{
}

#if defined(MA_HAS_RVALUE_REFS)

console_signal_service::post_adapter::post_adapter(const this_type& other)
  : handlers_(other.handlers_)
{
}

console_signal_service::post_adapter::post_adapter(this_type&& other)
  : handlers_(std::move(other.handlers_))
{
}

#endif // defined(MA_HAS_RVALUE_REFS)

void console_signal_service::post_adapter::operator()()
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
  handler_list_guard handlers;  
  {
    lock_guard lock(mutex_);
    if (!shutdown_)
    {
      // Take ownership of waiting handlers
#if defined(MA_HAS_RVALUE_REFS)
      handlers.list = std::move(impl.handlers_);
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
  handler_list_guard handlers;
  {
    lock_guard lock(mutex_);
    if (!shutdown_)
    {
      // Take ownership of waiting handlers
#if defined(MA_HAS_RVALUE_REFS)
      handlers.list = std::move(impl.handlers_);
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
  handler_list_guard handlers;
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

void console_signal_service::deliver_signal()
{
  post_adapter::handler_list_guard_ptr handlers =
      boost::make_shared<handler_list_guard>();
  {  
    lock_guard lock(mutex_);
    if (!shutdown_)
    {
      // Take ownership of all still active handlers.
      for (impl_base* impl = impl_list_.front(); impl;
          impl = impl_list_.next(*impl))
      {      
        handlers->list.insert_front(impl->handlers_);
      }
    }
  }
  if (!handlers->list.empty())
  {
    get_io_service().post(post_adapter(handlers));
  }
}

} // namespace windows
} // namespace ma
