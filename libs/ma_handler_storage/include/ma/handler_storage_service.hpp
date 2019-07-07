//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_SERVICE_HPP
#define MA_HANDLER_STORAGE_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/throw_exception.hpp>
#include <ma/config.hpp>
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
  // Base class to hold up any value.
  class stored_base;

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
    stored_base* handler_;
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

  template <typename Handler, typename Arg, typename Target>
  void store(implementation_type& impl, Handler handler);

  template <typename Arg, typename Target>
  static void post(implementation_type& impl, const Arg& arg);

  template <typename Target>
  static void post(implementation_type& impl);

  template <typename Arg, typename Target>
  static Target* target(const implementation_type& impl);

  static bool empty(const implementation_type& impl);
  static bool has_target(const implementation_type& impl);

protected:
  virtual ~handler_storage_service();

private:
  // Base class to hold up handlers with specified call signature.
  template <typename Arg, typename Target>
  class handler_base;

  template <typename Target>
  class handler_base<void, Target>;

  // Wrapper class to hold up handlers with specified call signature.
  template <typename Handler, typename Arg, typename Target>
  class handler_wrapper;

  template <typename Handler, typename Target>
  class handler_wrapper<Handler, void, Target>;

  typedef detail::mutex                     mutex_type;
  typedef detail::lock_guard<mutex_type>    lock_guard;
  typedef detail::intrusive_list<impl_base> impl_base_list;

  virtual void shutdown_service();

  // Guard for the impl_list_
  mutex_type impl_list_mutex_;
  // Double-linked intrusive list of active implementations.
  impl_base_list impl_list_;
  // Shutdown state flag.
  bool shutdown_;
}; // class handler_storage_service

inline bad_handler_call::bad_handler_call()
  : std::runtime_error("call to empty ma::handler_storage")
{
}

class handler_storage_service::stored_base
  : public detail::intrusive_forward_list<stored_base>::base_hook
{
private:
  typedef stored_base this_type;
  typedef detail::intrusive_forward_list<stored_base>::base_hook base_type;

public:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  virtual void destroy() = 0;

#else

  void destroy();

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

protected:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  stored_base();

#else

  typedef void (*destroy_func_type)(this_type*);

  stored_base(destroy_func_type);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  ~stored_base();
  stored_base(const this_type&);

private:
  this_type& operator=(const this_type&);

#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  destroy_func_type destroy_func_;
#endif
}; // class handler_storage_service::stored_base

template <typename Arg, typename Target>
class handler_storage_service::handler_base : public stored_base
{
private:
  typedef handler_base<Arg, Target> this_type;
  typedef stored_base base_type;

public:
  typedef Target target_type;

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  virtual void post(const Arg&) = 0;
  virtual target_type* target() = 0;

#else

  void post(const Arg&);
  target_type* target();

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

protected:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  handler_base();

#else

  typedef void (*post_func_type)(this_type*, const Arg&);
  typedef target_type* (*target_func_type)(this_type*);

  handler_base(destroy_func_type, post_func_type, target_func_type);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  ~handler_base();
  handler_base(const this_type&);

private:
  this_type& operator=(const this_type&);

#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  post_func_type   post_func_;
  target_func_type target_func_;

#endif // defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
}; // class handler_storage_service::handler_base

template <typename Target>
class handler_storage_service::handler_base<void, Target>
  : public stored_base
{
private:
  typedef handler_base<void, Target> this_type;
  typedef stored_base base_type;

public:
  typedef Target target_type;

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  virtual void post() = 0;
  virtual target_type* target() = 0;

#else

  void post();
  target_type* target();

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

protected:

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  handler_base();

#else

  typedef void (*post_func_type)(this_type*);
  typedef target_type* (*target_func_type)(this_type*);

  handler_base(destroy_func_type, post_func_type, target_func_type);

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  ~handler_base();
  handler_base(const this_type&);

private:
  this_type& operator=(const this_type&);

#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  post_func_type   post_func_;
  target_func_type target_func_;

#endif // defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
}; // class handler_storage_service::handler_base

template <typename Handler, typename Arg, typename Target>
class handler_storage_service::handler_wrapper
  : public handler_base<Arg, Target>
{
private:
  typedef handler_wrapper<Handler, Arg, Target> this_type;
  typedef handler_base<Arg, Target> base_type;

public:
  typedef typename base_type::target_type target_type;

  template <typename H>
  handler_wrapper(boost::asio::io_service&, MA_FWD_REF(H));

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  handler_wrapper(this_type&&);
  handler_wrapper(const this_type&);

#endif

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  virtual void destroy();
  virtual void post(const Arg&);
  virtual target_type* target();

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

#if !defined(NDEBUG)
  ~handler_wrapper();
#endif

private:
  this_type& operator=(const this_type&);

  static void do_destroy(stored_base*);
  static void do_post(base_type*, const Arg&);
  static target_type* do_target(base_type*);

  boost::asio::io_service::work work_;
  Handler handler_;
}; // class handler_storage_service::handler_wrapper

template <typename Handler, typename Target>
class handler_storage_service::handler_wrapper<Handler, void, Target>
  : public handler_base<void, Target>
{
private:
  typedef handler_wrapper<Handler, void, Target> this_type;
  typedef handler_base<void, Target> base_type;

public:
  typedef typename base_type::target_type target_type;

  template <typename H>
  handler_wrapper(boost::asio::io_service&, MA_FWD_REF(H));

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

  handler_wrapper(this_type&&);
  handler_wrapper(const this_type&);

#endif

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

  virtual void destroy();
  virtual void post();
  virtual target_type* target();

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

#if !defined(NDEBUG)
  ~handler_wrapper();
#endif

private:
  this_type& operator=(const this_type&);

  static void do_destroy(stored_base*);
  static void do_post(base_type*);
  static target_type* do_target(base_type*);

  boost::asio::io_service::work work_;
  Handler handler_;
}; // class handler_storage_service::handler_wrapper

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

inline handler_storage_service::stored_base::stored_base()
{
}

#else

inline void handler_storage_service::stored_base::destroy()
{
  destroy_func_(this);
}

inline handler_storage_service::stored_base::stored_base(
    destroy_func_type destroy_func)
  : destroy_func_(destroy_func)
{
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

inline handler_storage_service::stored_base::~stored_base()
{
}

inline handler_storage_service::stored_base::stored_base(
    const this_type& other)
  : base_type(other)
#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  , destroy_func_(other.destroy_func_)
#endif
{
}

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Arg, typename Target>
handler_storage_service::handler_base<Arg, Target>::handler_base()
  : base_type()
{
}

#else // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Arg, typename Target>
void handler_storage_service::handler_base<Arg, Target>::post(
    const Arg& arg)
{
  post_func_(this, arg);
}

template <typename Arg, typename Target>
typename handler_storage_service::handler_base<Arg, Target>::target_type*
handler_storage_service::handler_base<Arg, Target>::target()
{
  return target_func_(this);
}

template <typename Arg, typename Target>
handler_storage_service::handler_base<Arg, Target>::handler_base(
    destroy_func_type destroy_func, post_func_type post_func,
    target_func_type target_func)
  : base_type(destroy_func)
  , post_func_(post_func)
  , target_func_(target_func)
{
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Arg, typename Target>
handler_storage_service::handler_base<Arg, Target>::~handler_base()
{
}

template <typename Arg, typename Target>
handler_storage_service::handler_base<Arg, Target>::handler_base(
    const this_type& other)
  : base_type(other)
#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  , post_func_(other.post_func_)
  , target_func_(other.target_func_)
#endif
{
}

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Target>
handler_storage_service::handler_base<void, Target>::handler_base()
  : base_type()
{
}

#else // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Target>
void handler_storage_service::handler_base<void, Target>::post()
{
  post_func_(this);
}

template <typename Target>
typename handler_storage_service::handler_base<void, Target>::target_type*
handler_storage_service::handler_base<void, Target>::target()
{
  return target_func_(this);
}

template <typename Target>
handler_storage_service::handler_base<void, Target>::handler_base(
    destroy_func_type destroy_func, post_func_type post_func,
    target_func_type target_func)
  : base_type(destroy_func)
  , post_func_(post_func)
  , target_func_(target_func)
{
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Target>
handler_storage_service::handler_base<void, Target>::~handler_base()
{
}

template <typename Target>
handler_storage_service::handler_base<void, Target>::handler_base(
    const this_type& other)
  : base_type(other)
#if defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  , post_func_(other.post_func_)
  , target_func_(other.target_func_)
#endif
{
}

template <typename Handler, typename Arg, typename Target>
template <typename H>
handler_storage_service::handler_wrapper<Handler, Arg, Target>::handler_wrapper(
    boost::asio::io_service& io_service, MA_FWD_REF(H) handler)
#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  : base_type()
#else
  : base_type(&this_type::do_destroy, &this_type::do_post,
        &this_type::do_target)
#endif
  , work_(io_service)
  , handler_(detail::forward<H>(handler))
{
}

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

template <typename Handler, typename Arg, typename Target>
handler_storage_service::handler_wrapper<Handler, Arg, Target>::handler_wrapper(
    this_type&& other)
  : base_type(detail::move(other))
  , work_(detail::move(other.work_))
  , handler_(detail::move(other.handler_))
{
}

template <typename Handler, typename Arg, typename Target>
handler_storage_service::handler_wrapper<Handler, Arg, Target>::handler_wrapper(
    const this_type& other)
  : base_type(other)
  , work_(other.work_)
  , handler_(other.handler_)
{
}

#endif

#if !defined(NDEBUG)

template <typename Handler, typename Arg, typename Target>
handler_storage_service::handler_wrapper<Handler, Arg, Target>::
    ~handler_wrapper()
{
}

#endif // !defined(NDEBUG)

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Handler, typename Arg, typename Target>
void handler_storage_service::handler_wrapper<Handler, Arg, Target>::destroy()
{
  do_destroy(this);
}

template <typename Handler, typename Arg, typename Target>
void handler_storage_service::handler_wrapper<Handler, Arg, Target>::post(
    const Arg& arg)
{
  do_post(this, arg);
}

template <typename Handler, typename Arg, typename Target>
typename handler_storage_service::handler_wrapper<Handler, Arg, Target>::
    target_type*
handler_storage_service::handler_wrapper<Handler, Arg, Target>::target()
{
  return do_target(this);
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Handler, typename Arg, typename Target>
void handler_storage_service::handler_wrapper<Handler, Arg, Target>::do_destroy(
    stored_base* base)
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

template <typename Handler, typename Arg, typename Target>
void handler_storage_service::handler_wrapper<Handler, Arg, Target>::do_post(
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
  io_service.post(ma::bind_handler(detail::move(handler), arg));
}

template <typename Handler, typename Arg, typename Target>
typename handler_storage_service::handler_wrapper<Handler, Arg, Target>
    ::target_type*
handler_storage_service::handler_wrapper<Handler, Arg, Target>::do_target(
    base_type* base)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  return static_cast<target_type*>(detail::addressof(this_ptr->handler_));
}

template <typename Handler, typename Target>
template <typename H>
handler_storage_service::handler_wrapper<Handler, void, Target>::
    handler_wrapper(boost::asio::io_service& io_service, MA_FWD_REF(H) handler)
#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)
  : base_type()
#else
  : base_type(&this_type::do_destroy, &this_type::do_post,
        &this_type::do_target)
#endif
  , work_(io_service)
  , handler_(detail::forward<H>(handler))
{
}

#if defined(MA_HAS_RVALUE_REFS) \
    && (defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG))

template <typename Handler, typename Target>
handler_storage_service::handler_wrapper<Handler, void, Target>::
    handler_wrapper(this_type&& other)
  : base_type(detail::move(other))
  , work_(detail::move(other.work_))
  , handler_(detail::move(other.handler_))
{
}

template <typename Handler, typename Target>
handler_storage_service::handler_wrapper<Handler, void, Target>::
    handler_wrapper(const this_type& other)
  : base_type(other)
  , work_(other.work_)
  , handler_(other.handler_)
{
}

#endif

#if !defined(NDEBUG)

template <typename Handler, typename Target>
handler_storage_service::handler_wrapper<Handler, void, Target>::
    ~handler_wrapper()
{
}

#endif // !defined(NDEBUG)

#if !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Handler, typename Target>
void handler_storage_service::handler_wrapper<Handler, void, Target>::destroy()
{
  do_destroy(this);
}

template <typename Handler, typename Target>
void handler_storage_service::handler_wrapper<Handler, void, Target>::post()
{
  do_post(this);
}

template <typename Handler, typename Target>
typename handler_storage_service::handler_wrapper<Handler, void, Target>::
    target_type*
handler_storage_service::handler_wrapper<Handler, void, Target>::target()
{
  return do_target(this);
}

#endif // !defined(MA_TYPE_ERASURE_NOT_USE_VIRTUAL)

template <typename Handler, typename Target>
void handler_storage_service::handler_wrapper<Handler, void, Target>::
    do_destroy(stored_base* base)
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

template <typename Handler, typename Target>
void handler_storage_service::handler_wrapper<Handler, void, Target>::do_post(
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
  io_service.post(detail::move(handler));
}

template <typename Handler, typename Target>
typename handler_storage_service::handler_wrapper<Handler, void, Target>
    ::target_type*
handler_storage_service::handler_wrapper<Handler, void, Target>::do_target(
    base_type* base)
{
  this_type* this_ptr = static_cast<this_type*>(base);
  return static_cast<target_type*>(detail::addressof(this_ptr->handler_));
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
  , shutdown_(false)
{
}

inline void handler_storage_service::construct(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }

  // Add implementation to the list of active implementations.
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.push_back(impl);
  }
}

inline void handler_storage_service::move_construct(implementation_type& impl,
    implementation_type& other_impl)
{
  if (shutdown_)
  {
    return;
  }

  // Add implementation to the list of active implementations.
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.push_back(impl);
  }

  // Move ownership of the stored handler
  impl.handler_ = other_impl.handler_;
  other_impl.handler_ = 0;
}

inline void handler_storage_service::destroy(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }

  // Remove implementation from the list of active implementations.
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.erase(impl);
  }

  // Destroy stored handler if it exists.
  clear(impl);
}

inline void handler_storage_service::clear(implementation_type& impl)
{
  // Destroy stored handler if it exists.
  if (stored_base* handler = impl.handler_)
  {
    impl.handler_ = 0;
    handler->destroy();
  }
}

template <typename Handler, typename Arg, typename Target>
void handler_storage_service::store(implementation_type& impl, Handler handler)
{
  // If service is (was) in shutdown state then it can't store handler.
  if (shutdown_)
  {
    return;
  }

  typedef typename detail::decay<Arg>::type           arg_type;
  typedef typename detail::decay<Target>::type        target_type;
  typedef handler_wrapper<Handler, arg_type, target_type>   value_type;
  typedef detail::handler_alloc_traits<Handler, value_type> alloc_traits;

  // Allocate raw memory for storing the handler
  detail::raw_handler_ptr<alloc_traits> raw_ptr(handler);
  // Create wrapped handler at allocated memory and
  // move ownership of allocated memory to ptr
  detail::handler_ptr<alloc_traits> ptr(raw_ptr,
      detail::ref(ma::get_io_context(*this)), detail::move(handler));
  // Copy current handler
  stored_base* old_handler = impl.handler_;
  // Move ownership of already created wrapped handler
  // (and allocated memory) to the impl
  impl.handler_ = ptr.release();
  // Destroy previously stored handler
  if (old_handler)
  {
    old_handler->destroy();
  }
}

template <typename Arg, typename Target>
void handler_storage_service::post(implementation_type& impl, const Arg& arg)
{
  typedef typename detail::decay<Arg>::type    arg_type;
  typedef typename detail::decay<Target>::type target_type;
  typedef handler_base<arg_type, target_type>        handler_type;

  if (handler_type* handler = static_cast<handler_type*>(impl.handler_))
  {
    impl.handler_ = 0;
    handler->post(arg);
  }
  else
  {
    boost::throw_exception(bad_handler_call());
  }
}

template <typename Target>
void handler_storage_service::post(implementation_type& impl)
{
  typedef void arg_type;
  typedef typename detail::decay<Target>::type target_type;
  typedef handler_base<arg_type, target_type>        handler_type;

  if (handler_type* handler = static_cast<handler_type*>(impl.handler_))
  {
    impl.handler_ = 0;
    handler->post();
  }
  else
  {
    boost::throw_exception(bad_handler_call());
  }
}

template <typename Arg, typename Target>
Target* handler_storage_service::target(const implementation_type& impl)
{
  typedef typename detail::decay<Arg>::type    arg_type;
  typedef typename detail::decay<Target>::type target_type;
  typedef handler_base<arg_type, target_type>        handler_type;

  if (handler_type* handler = static_cast<handler_type*>(impl.handler_))
  {
    return handler->target();
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
  BOOST_ASSERT_MSG(shutdown_, "shutdown_service() was not called");
}

inline void handler_storage_service::shutdown_service()
{
  // Restrict usage of service.
  shutdown_ = true;
  // Take ownership of all still active handlers.
  detail::intrusive_forward_list<stored_base> handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {
      if (stored_base* handler = impl->handler_)
      {
        handlers.push_front(*handler);
        impl->handler_ = 0;
      }
    }
  }
  // Destroy all handlers
  for (stored_base* handler = handlers.front(); handler; )
  {
    stored_base* next_handler = handlers.next(*handler);
    handler->destroy();
    handler = next_handler;
  }
}

} // namespace ma

#endif // MA_HANDLER_STORAGE_SERVICE_HPP
