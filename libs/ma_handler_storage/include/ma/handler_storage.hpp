//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_HPP
#define MA_HANDLER_STORAGE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/handler_storage_service.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

/// Provides storage for handlers.
/**
 * The handler_storage class provides the storage for handlers:
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/Handler.html
 * It supports Boost.Asio custom memory allocation:
 * http://www.boost.org/doc/libs/release/doc/html/boost_asio/overview/core/allocation.html
 *
 * A value h of a stored handler class should work correctly
 * in the expression h(arg) where arg is an lvalue of type const Arg.
 *
 * Stored handler must have nothrow copy-constructor.
 * This restriction is predicted by asio custom memory allocation.
 *
 * Every instance of handler_storage class is tied to
 * some instance of boost::asio::io_service class.
 *
 * The stored handler can't be invoked (and must not be invoked) directly.
 * It can be only destroyed or posted (with immediate stored value
 * destruction) to the io_service object to which the handler_storage object
 * is tied by usage of boost::asio::io_service::post method.
 *
 * The handler_storage class instances are automatically cleaned up
 * during destruction of the tied io_service (those handler_storage class
 * instances that are alive to that moment).
 * That clean up is done by destruction of the stored value (handler) -
 * not the handler_storage instance itself.
 * Because of the automatic clean up, users of handler_storage must remember
 * that the stored value (handler) may be destroyed without explicit
 * user activity. Also this implies to the thread safety.
 * The handler_storage instances must not outlive the tied io_service object.
 * A handler_storage object can store a value (handler) that is
 * the owner of the handler_storage object itself.
 * This is one of the reasons of automatic clean up.
 *
 * handler_storage is like boost::function, except:
 *
 * @li boost::function is more flexible and general,
 * @li handler_storage supports Boost.Asio custom memory allocation,
 * @li handler_storage is automatically cleaned up during io_service
 * destruction,
 * @li handler_storage is noncopyable.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe until boost::asio::io_service::~io_service().@n
 * @e Shared @e objects: Unsafe.
 *
 * At the point of execution of io_service::~io_service() access to all members
 * of any handler_storage instance may be done only within context
 * of the thread executing io_service::~io_service().
 *
 * From the start point of io_service::~io_service() it is not guaranted that
 * store(handler) can store handler. If underlying service was shut down then
 * store(handler) won't do anything at all.
 */
template <typename Arg, typename Target = void>
class handler_storage : private boost::noncopyable
{
private:
  typedef handler_storage<Arg, Target> this_type;

public:
  typedef handler_storage_service                    service_type;
  typedef typename service_type::implementation_type implementation_type;
  typedef typename detail::decay<Arg>::type          arg_type;
  typedef typename detail::decay<Target>::type       target_type;

  explicit handler_storage(boost::asio::io_service& io_service);
  ~handler_storage();

#if defined(MA_HAS_RVALUE_REFS)
  handler_storage(this_type&& other);
#endif

  boost::asio::io_service& get_io_service();

  /// Get pointer to the stored handler.
  /**
   * Because of type erasure it's "pointer to void" so "reinterpret_cast"
   * should be used. See usage example at "nmea_client" project.
   * If storage doesn't contain any handler then returns null pointer.
   */
  target_type* target();

  /// Get pointer to the stored handler. Const version.
  const target_type* target() const;

  /// Check if handler storage is empty (doesn't contain any handler).
  /**
   * It doesn't clear handler storage. See frequent STL-related errors
   * at PVS-Studio site 8) - it's not an advertisement but really interesting
   * reading.
   */
  bool empty() const;

  /// Check if handler storage contains handler.
  bool has_target() const;

  /// Clear stored handler if it exists.
  void clear();

  /// Store handler in this handler storage.
  /**
   * Really, "store" means "try to store, if can't (io_service's destructor is
   * already called) then do nothing".
   * For test of was "store" successful or not, "has_target" can be used
   * (called right after "store").
   */
  template <typename Handler>
  void store(MA_FWD_REF(Handler) handler);

  /// Post the stored handler to storage related io_service instance.
  /**
   * Attention!
   * Always check if handler storage has any handler stored in it.
   * Use "has_target". Always - even if you already have called "store" method.
   * Really, "store" means "try to store, if can't (io_service's destructor is
   * already called) then do nothing".
   */
  void post(const arg_type& arg);

private:
  service_type&       service_;
  implementation_type impl_;
}; // class handler_storage

template <typename Target>
class handler_storage<void, Target> : private boost::noncopyable
{
private:
  typedef handler_storage<void, Target> this_type;

public:
  typedef handler_storage_service                    service_type;
  typedef typename service_type::implementation_type implementation_type;
  typedef void                                       arg_type;
  typedef typename detail::decay<Target>::type       target_type;

  explicit handler_storage(boost::asio::io_service& io_service);
  ~handler_storage();

#if defined(MA_HAS_RVALUE_REFS)
  handler_storage(this_type&& other);
#endif

  boost::asio::io_service& get_io_service();

  /// Get pointer to the stored handler.
  /**
   * Because of type erasure it's "pointer to void" so "reinterpret_cast"
   * should be used. See usage example at "nmea_client" project.
   * If storage doesn't contain any handler then returns null pointer.
   */
  target_type* target();

  /// Get pointer to the stored handler. Const version.
  const target_type* target() const;

  /// Check if handler storage is empty (doesn't contain any handler).
  /**
   * It doesn't clear handler storage. See frequent STL-related errors
   * at PVS-Studio site 8) - it's not an advertisement but really interesting
   * reading.
   */
  bool empty() const;

  /// Check if handler storage contains handler.
  bool has_target() const;

  /// Clear stored handler if it exists.
  void clear();

  /// Store handler in this handler storage.
  /**
   * Really, "store" means "try to store, if can't (io_service's destructor is
   * already called) then do nothing".
   * For test of was "store" successful or not, "has_target" can be used
   * (called right after "store").
   */
  template <typename Handler>
  void store(MA_FWD_REF(Handler) handler);

  /// Post the stored handler to storage related io_service instance.
  /**
   * Attention!
   * Alwasy check if handler storage has any handler stored in it.
   * Use "has_target". Always - even if you already have called "store" method.
   * Really, "store" means "try to store, if can't (io_service's destructor is
   * already called) then do nothing".
   */
  void post();

private:
  service_type&       service_;
  implementation_type impl_;
}; // class handler_storage

template <typename Arg, typename Target>
handler_storage<Arg, Target>::handler_storage(
    boost::asio::io_service& io_service)
  : service_(boost::asio::use_service<service_type>(io_service))
{
  service_.construct(impl_);
}

template <typename Arg, typename Target>
handler_storage<Arg, Target>::~handler_storage()
{
  service_.destroy(impl_);
}

#if defined(MA_HAS_RVALUE_REFS)

template <typename Arg, typename Target>
handler_storage<Arg, Target>::handler_storage(this_type&& other)
  : service_(other.service_)
{
  service_.move_construct(impl_, other.impl_);
}

#endif // defined(MA_HAS_RVALUE_REFS)

template <typename Arg, typename Target>
boost::asio::io_service& handler_storage<Arg, Target>::get_io_service()
{
  return ma::get_io_context(service_);
}

template <typename Arg, typename Target>
typename handler_storage<Arg, Target>::target_type*
handler_storage<Arg, Target>::target()
{
  return service_.target<arg_type, target_type>(impl_);
}

template <typename Arg, typename Target>
const typename handler_storage<Arg, Target>::target_type*
handler_storage<Arg, Target>::target() const
{
  return service_.target<arg_type, target_type>(impl_);
}

template <typename Arg, typename Target>
bool handler_storage<Arg, Target>::empty() const
{
  return service_.empty(impl_);
}

template <typename Arg, typename Target>
bool handler_storage<Arg, Target>::has_target() const
{
  return service_.has_target(impl_);
}

template <typename Arg, typename Target>
void handler_storage<Arg, Target>::clear()
{
  service_.clear(impl_);
}

template <typename Arg, typename Target>
template <typename Handler>
void handler_storage<Arg, Target>::store(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  service_.store<handler_type, arg_type, target_type>(
      impl_, detail::forward<Handler>(handler));
}

template <typename Arg, typename Target>
void handler_storage<Arg, Target>::post(const arg_type& arg)
{
  service_.post<arg_type, target_type>(impl_, arg);
}

template <typename Target>
handler_storage<void, Target>::handler_storage(
    boost::asio::io_service& io_service)
  : service_(boost::asio::use_service<service_type>(io_service))
{
  service_.construct(impl_);
}

template <typename Target>
handler_storage<void, Target>::~handler_storage()
{
  service_.destroy(impl_);
}

#if defined(MA_HAS_RVALUE_REFS)

template <typename Target>
handler_storage<void, Target>::handler_storage(this_type&& other)
  : service_(other.service_)
{
  service_.move_construct(impl_, other.impl_);
}

#endif // defined(MA_HAS_RVALUE_REFS)

template <typename Target>
boost::asio::io_service& handler_storage<void, Target>::get_io_service()
{
  return ma::get_io_context(service_);
}

template <typename Target>
typename handler_storage<void, Target>::target_type*
handler_storage<void, Target>::target()
{
  return service_.target<void, target_type>(impl_);
}

template <typename Target>
const typename handler_storage<void, Target>::target_type*
handler_storage<void, Target>::target() const
{
  return service_.target<void, target_type>(impl_);
}

template <typename Target>
bool handler_storage<void, Target>::empty() const
{
  return service_.empty(impl_);
}

template <typename Target>
bool handler_storage<void, Target>::has_target() const
{
  return service_.has_target(impl_);
}

template <typename Target>
void handler_storage<void, Target>::clear()
{
  service_.clear(impl_);
}

template <typename Target>
template <typename Handler>
void handler_storage<void, Target>::store(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  service_.store<handler_type, void, target_type>(
      impl_, detail::forward<Handler>(handler));
}

template <typename Target>
void handler_storage<void, Target>::post()
{
  service_.post<target_type>(impl_);
}

} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP
