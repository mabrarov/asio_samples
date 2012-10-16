//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_SERVICE_HPP
#define MA_HANDLER_STORAGE_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/throw_exception.hpp>
#include <ma/config.hpp>
#include <ma/type_traits.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/handler_alloc_helpers.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {

namespace detail {

/// Simplified double-linked intrusive list.
/**
 * Requirements:
 * if value is rvalue of type Value then expression
 * static_cast&lt;intrusive_list&lt;Value&gt;::base_hook&amp;&gt;(Value)
 * must be well formed and accessible from intrusive_list&lt;Value&gt;.
 */
template<typename Value>
class intrusive_list : private boost::noncopyable
{
public:
  typedef Value  value_type;
  typedef Value* pointer;
  typedef Value& reference;

  /// Required hook for items of the list.
  class base_hook;

  /// Never throws
  intrusive_list();

  /// Never throws
  pointer front() const;

  /// Never throws
  static pointer prev(reference value);

  /// Never throws
  static pointer next(reference value);

  /// Never throws
  void push_front(reference value);

  /// Never throws
  void erase(reference value);

  /// Never throws
  void pop_front();

private:
  static base_hook& get_hook(reference value);

  pointer front_;
}; // class intrusive_list

template<typename Value>
class intrusive_list<Value>::base_hook
{
private:
  typedef base_hook this_type;

public:
  base_hook();
  base_hook(const this_type&);

private:
  friend class intrusive_list<Value>;
  pointer prev_;
  pointer next_;
}; // class intrusive_list::base_hook

// Special derived service id type to keep classes header-file only.
// Copied from boost/asio/io_service.hpp.
template <typename Type>
class service_id : public boost::asio::io_service::id
{
}; // class service_id

// Special service base class to keep classes header-file only.
// Copied from boost/asio/io_service.hpp.
template <typename Type>
class service_base : public boost::asio::io_service::service
{
public:
  static service_id<Type> id;

  explicit service_base(boost::asio::io_service& io_service);

protected:
  virtual ~service_base();
}; // class service_base

} // namespace detail

/// Exception thrown when handler_storage::post is used with empty
/// handler_storage.
class bad_handler_call : public std::runtime_error
{
public:
  bad_handler_call();
}; // class bad_handler_call

/// asio::io_service::service implementing handler_storage.
class handler_storage_service
  : public detail::service_base<handler_storage_service>
{
private:
  // Base class to hold up handlers.
  class handler_base;

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
    friend class handler_storage_service;
    // Pointer to the stored handler otherwise null pointer.
    handler_base* handler_;
  }; // class impl_base

public:
  class implementation_type : private impl_base
  {
  private:
    friend class handler_storage_service;
  }; // class implementation_type

  explicit handler_storage_service(boost::asio::io_service& io_service);
  void construct(implementation_type& impl);
  void move_construct(implementation_type& impl,
      implementation_type& other_impl);
  void destroy(implementation_type& impl);

  /// Can throw if destructor of user supplied handler can throw
  static void clear(implementation_type& impl);

  template <typename Handler, typename Arg>
  void store(implementation_type& impl, Handler handler);

  template <typename Arg>
  static void post(implementation_type& impl, const Arg& arg);

  static void* target(const implementation_type& impl);
  static bool empty(const implementation_type& impl);
  static bool has_target(const implementation_type& impl);

protected:
  virtual ~handler_storage_service();

private:
  // Base class to hold up handlers with specified call signature.
  template <typename Arg>
  class postable_handler_base;

  // Wrapper class to hold up handlers with specified call signature.
  template <typename Handler, typename Arg>
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
  bool shutdown_done_;
}; // class handler_storage_service

namespace detail {

template<typename Value>
intrusive_list<Value>::base_hook::base_hook()
  : prev_(0)
  , next_(0)
{
}

template<typename Value>
intrusive_list<Value>::base_hook::base_hook(const this_type& other)
  : prev_(0)
  , next_(0)
{
}

template<typename Value>
intrusive_list<Value>::intrusive_list()
  : front_(0)
{
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::front() const
{
  return front_;
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::prev(reference value)
{
  return get_hook(value).prev_;
}

template<typename Value>
typename intrusive_list<Value>::pointer
intrusive_list<Value>::next(reference value)
{
  return get_hook(value).next_;
}

template<typename Value>
void intrusive_list<Value>::push_front(reference value)
{
  base_hook& value_hook = get_hook(value);

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The value to push has to be not linked");

  value_hook.next_ = front_;
  if (value_hook.next_)
  {
    base_hook& front_hook = get_hook(*value_hook.next_);
    front_hook.prev_ = boost::addressof(value);
  }
  front_ = boost::addressof(value);
}

template<typename Value>
void intrusive_list<Value>::erase(reference value)
{
  base_hook& value_hook = get_hook(value);
  if (front_ == boost::addressof(value))
  {
    front_ = value_hook.next_;
  }
  if (value_hook.prev_)
  {
    base_hook& prev_hook = get_hook(*value_hook.prev_);
    prev_hook.next_ = value_hook.next_;
  }
  if (value_hook.next_)
  {
    base_hook& next_hook = get_hook(*value_hook.next_);
    next_hook.prev_ = value_hook.prev_;
  }
  value_hook.prev_ = value_hook.next_ = 0;

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The erased value has to be unlinked");
}

template<typename Value>
void intrusive_list<Value>::pop_front()
{
  BOOST_ASSERT_MSG(front_, "The container is empty");

  base_hook& value_hook = get_hook(*front_);
  front_ = value_hook.next_;
  if (front_)
  {
    base_hook& front_hook = get_hook(*front_);
    front_hook.prev_= 0;
  }
  value_hook.next_ = value_hook.prev_ = 0;

  BOOST_ASSERT_MSG(!value_hook.prev_ && !value_hook.next_,
      "The popped value has to be unlinked");
}

template<typename Value>
typename intrusive_list<Value>::base_hook&
intrusive_list<Value>::get_hook(reference value)
{
  return static_cast<base_hook&>(value);
}

template <typename Type>
service_base<Type>::service_base(boost::asio::io_service& io_service)
  : boost::asio::io_service::service(io_service)
{
}

template <typename Type>
service_base<Type>::~service_base()
{
}

template <typename Type>
service_id<Type> service_base<Type>::id;

} // namespace detail

inline bad_handler_call::bad_handler_call()
  : std::runtime_error("call to empty ma::handler_storage")
{
}

class handler_storage_service::handler_base
  : public detail::intrusive_list<handler_base>::base_hook
{
private:
  typedef handler_base this_type;

public:
  typedef void (*destroy_func_type)(this_type*);
  typedef void* (*target_func_type)(this_type*);

  handler_base(destroy_func_type, target_func_type);
  void destroy();
  void* target();

protected:
  ~handler_base();

private:
  destroy_func_type destroy_func_;
  target_func_type  target_func_;
}; // class handler_storage_service::handler_base

template <typename Arg>
class handler_storage_service::postable_handler_base : public handler_base
{
private:
  typedef postable_handler_base<Arg> this_type;

public:
  typedef void (*post_func_type)(this_type*, const Arg&);

  postable_handler_base(destroy_func_type, target_func_type, post_func_type);
  void post(const Arg&);

protected:
  ~postable_handler_base();

private:
  post_func_type post_func_;
}; // class handler_storage_service::postable_handler_base

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4512)
#endif // #if defined(_MSC_VER)

template <typename Handler, typename Arg>
class handler_storage_service::handler_wrapper
  : public postable_handler_base<Arg>
{
private:
  typedef handler_wrapper<Handler, Arg> this_type;
  typedef postable_handler_base<Arg>    base_type;

public:

#if defined(MA_HAS_RVALUE_REFS)

  template <typename H>
  handler_wrapper(boost::asio::io_service&, H&&);

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  handler_wrapper(this_type&&);
  handler_wrapper(const this_type&);

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

#else // defined(MA_HAS_RVALUE_REFS)

  handler_wrapper(boost::asio::io_service&, const Handler&);

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)
  ~handler_wrapper();
#endif

  static void do_post(base_type*, const Arg&);
  static void do_destroy(handler_base*);
  static void* do_target(handler_base*);

private:
  boost::asio::io_service& io_service_;
  boost::asio::io_service::work work_;
  Handler handler_;
}; // class handler_storage_service::handler_wrapper

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // #if defined(_MSC_VER)

inline handler_storage_service::handler_base::handler_base(
    destroy_func_type destroy_func, target_func_type target_func)
  : destroy_func_(destroy_func)
  , target_func_(target_func)
{
}

inline void handler_storage_service::handler_base::destroy()
{
  destroy_func_(this);
}

inline void* handler_storage_service::handler_base::target()
{
  return target_func_(this);
}

inline handler_storage_service::handler_base::~handler_base()
{
}

template <typename Arg>
handler_storage_service::postable_handler_base<Arg>::postable_handler_base(
    destroy_func_type destroy_func, target_func_type target_func,
    post_func_type post_func)
  : handler_base(destroy_func, target_func)
  , post_func_(post_func)
{
}

template <typename Arg>
void handler_storage_service::postable_handler_base<Arg>::post(const Arg& arg)
{
  post_func_(this, arg);
}

template <typename Arg>
handler_storage_service::postable_handler_base<Arg>::~postable_handler_base()
{
}

#if defined(MA_HAS_RVALUE_REFS)

template <typename Handler, typename Arg> template <typename H>
handler_storage_service::handler_wrapper<Handler, Arg>::handler_wrapper(
    boost::asio::io_service& io_service, H&& handler)
  : base_type(&this_type::do_destroy, &this_type::do_target,
        &this_type::do_post)
  , io_service_(io_service)
  , work_(io_service)
  , handler_(std::forward<H>(handler))
{
}

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Handler, typename Arg>
handler_storage_service::handler_wrapper<Handler, Arg>::handler_wrapper(
    this_type&& other)
  : base_type(std::move(other))
  , io_service_(other.io_service_)
  , work_(std::move(other.work_))
  , handler_(std::move(other.handler_))
{
}

template <typename Handler, typename Arg>
handler_storage_service::handler_wrapper<Handler, Arg>::handler_wrapper(
    const this_type& other)
  : base_type(other)
  , io_service_(other.io_service_)
  , work_(other.work_)
  , handler_(other.handler_)
{
}

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Handler, typename Arg> template <typename H>
handler_storage_service::handler_wrapper<Handler, Arg>::handler_wrapper(
    boost::asio::io_service& io_service,const Handler& handler)
  : base_type(&this_type::do_destroy, &this_type::do_target,
        &this_type::do_post)
  , io_service_(io_service)
  , work_(io_service)
  , handler_(handler)
{
}

#endif // defined(MA_HAS_RVALUE_REFS)

#if !defined(NDEBUG)

template <typename Handler, typename Arg>
handler_storage_service::handler_wrapper<Handler, Arg>::~handler_wrapper()
{
}

#endif // !defined(NDEBUG)

template <typename Handler, typename Arg>
void handler_storage_service::handler_wrapper<Handler, Arg>::do_post(
    base_type* base, const Arg& arg)
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
  boost::asio::io_service& io_service(this_ptr->io_service_);
  boost::asio::io_service::work work(this_ptr->work_);
  (void) work;
  // Destroy wrapper object and deallocate its memory
  // through the local copy of handler
  ptr.reset();
  // Post the copy of handler's local copy to io_service
#if defined(MA_HAS_RVALUE_REFS)
  io_service.post(detail::bind_handler(std::move(handler), arg));
#else
  io_service.post(detail::bind_handler(handler, arg));
#endif
}

template <typename Handler, typename Arg>
void handler_storage_service::handler_wrapper<Handler, Arg>::do_destroy(
    handler_base* base)
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

template <typename Handler, typename Arg>
void* handler_storage_service::handler_wrapper<Handler, Arg>::do_target(
    handler_base* base)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  return boost::addressof(this_ptr->handler_);
}

inline handler_storage_service::impl_base::impl_base()
  : handler_(0)
{
}

#if !defined(NDEBUG)

inline handler_storage_service::impl_base::~impl_base()
{
  BOOST_ASSERT_MSG(!handler_, "The stored handler was not destroyed");
}

#endif // !defined(NDEBUG)

inline handler_storage_service::handler_storage_service(
    boost::asio::io_service& io_service)
  : detail::service_base<handler_storage_service>(io_service)
  , shutdown_done_(false)
{
}

inline void handler_storage_service::construct(implementation_type& impl)
{
  if (!shutdown_done_)
  {
    // Add implementation to the list of active implementations.
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.push_front(impl);
  }
}

inline void handler_storage_service::move_construct(implementation_type& impl,
    implementation_type& other_impl)
{
  if (!shutdown_done_)
  {
    // Add implementation to the list of active implementations.
    {
      lock_guard impl_list_lock(impl_list_mutex_);
      impl_list_.push_front(impl);
    }
    // Move ownership of the stored handler
    impl.handler_ = other_impl.handler_;
    other_impl.handler_ = 0;
  }
}

inline void handler_storage_service::destroy(implementation_type& impl)
{
  if (!shutdown_done_)
  {
    {
      // Remove implementation from the list of active implementations.
      lock_guard impl_list_lock(impl_list_mutex_);
      impl_list_.erase(impl);
    }
    // Destroy stored handler if it exists.
    clear(impl);
  }
}

inline void handler_storage_service::clear(implementation_type& impl)
{
  // Destroy stored handler if it exists.
  if (handler_base* handler = impl.handler_)
  {
    impl.handler_ = 0;
    handler->destroy();
  }
}

template <typename Handler, typename Arg>
void handler_storage_service::store(implementation_type& impl, Handler handler)
{
  // If service is (was) in shutdown state then it can't store handler.
  if (!shutdown_done_)
  {
    typedef typename remove_cv_reference<Arg>::type arg_type;
    typedef handler_wrapper<Handler, arg_type> value_type;
    typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;
    // Allocate raw memory for storing the handler
    detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
    // Create wrapped handler at allocated memory and
    // move ownership of allocated memory to ptr
#if defined(MA_HAS_RVALUE_REFS)
    detail::handler_ptr<alloc_traits> ptr(raw_ptr,
        boost::ref(this->get_io_service()), std::move(handler));
#else
    detail::handler_ptr<alloc_traits> ptr(raw_ptr,
        boost::ref(this->get_io_service()), handler);
#endif
    // Copy current handler
    handler_base* old_handler = impl.handler_;
    // Move ownership of already created wrapped handler
    // (and allocated memory) to the impl
    impl.handler_ = ptr.release();
    // Destroy previosly stored handler
    if (old_handler)
    {
      old_handler->destroy();
    }
  }
}

template <typename Arg>
void handler_storage_service::post(implementation_type& impl, const Arg& arg)
{
  typedef typename remove_cv_reference<Arg>::type arg_type;
  typedef postable_handler_base<arg_type> postable_base;
  if (postable_base* handler = static_cast<postable_base*>(impl.handler_))
  {
    impl.handler_ = 0;
    handler->post(arg);
  }
  else
  {
    boost::throw_exception(bad_handler_call());
  }
}

inline void* handler_storage_service::target(const implementation_type& impl)
{
  if (impl.handler_)
  {
    return impl.handler_->target();
  }
  return 0;
}

inline bool handler_storage_service::empty(const implementation_type& impl)
{
  return 0 == impl.handler_;
}

inline bool handler_storage_service::has_target(const implementation_type& impl)
{
  return 0 != impl.handler_;
}

inline handler_storage_service::~handler_storage_service()
{
}

inline void handler_storage_service::shutdown_service()
{
  // Restrict usage of service.
  shutdown_done_ = true;
  // Take ownership of all still active handlers.
  detail::intrusive_list<handler_base> stored_handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {
      if (handler_base* handler = impl->handler_)
      {
        stored_handlers.push_front(*handler);
        impl->handler_ = 0;
      }
    }
  }
  // Destroy all handlers
  for (handler_base* handler = stored_handlers.front(); handler; )
  {
    handler_base* next_handler = stored_handlers.next(*handler);
    handler->destroy();
    handler = next_handler;
  }
}

} // namespace ma

#endif // MA_HANDLER_STORAGE_SERVICE_HPP
