//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_STORAGE_HPP
#define MA_HANDLER_STORAGE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage_service.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {  

/// Provides storage for handlers.
/**
 * The handler_storage class provides the storage for handlers:
 * http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/reference/Handler.html   
 * It supports Boost.Asio custom memory allocation: 
 * http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/overview/core/allocation.html
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
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *   
 * Attention! 
 * Because of the speed decisions no additional run-time checks are done in
 * release version.
 */
template <typename Arg>
class handler_storage : private boost::noncopyable
{
public:
  typedef Arg arg_type;
  typedef handler_storage_service<arg_type>          service_type;
  typedef typename service_type::implementation_type implementation_type;

  explicit handler_storage(boost::asio::io_service& io_service)
    : service_(boost::asio::use_service<service_type>(io_service))
  {
    service_.construct(implementation_);
  }

  ~handler_storage()
  {
    service_.destroy(implementation_);
  } 

  /// boost::asio::basic_io_object-like member function.
  boost::asio::io_service& io_service()
  {
    return service_.get_io_service();
  }

  /// boost::asio::basic_io_object-like member function.
  boost::asio::io_service& get_io_service()
  {
    return service_.get_io_service();
  }

  /// Get pointer to the stored handler. 
  /**
   * Because of type erasure it's "pointer to void" so "reinterpret_cast"
   * should be used. See usage example at "nmea_client" project.
   * If storage doesn't contain any handler then returns null pointer.
   */
  void* target() const
  {
    return service_.target(implementation_);
  }

  /// Check if handler storage is empty (doesn't contain any handler).
  /** 
   * It doesn't clear handler storage. See frequent STL-related errors 
   * at PVS-Studio site 8) - it's not an advertisement but really interesting 
   * reading.
   */
  bool empty() const
  {
    return service_.empty(implementation_);
  }

  /// Check if handler storage contains handler.
  bool has_target() const
  {
    return service_.has_target(implementation_);
  }    

#if defined(MA_HAS_RVALUE_REFS)

  /// Store handler in this handler storage.
  /**
   * Attention!
   * Really, "reset" means "try to put, if can't (io_service's destructor is
   * already called) then do nothing".
   * For test of was "reset" successful or not, "has_target" can be used 
   * (called right after "reset").
   */
  template <typename Handler>
  void reset(Handler&& handler)
  {      
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    service_.reset<handler_type>(implementation_, 
        std::forward<Handler>(handler));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  /// Store handler in this handler storage.
  /**
   * Attention!
   * Really, "reset" means "try to put, if can't (io_service's destructor is
   * already called) then do nothing".
   * For test of was "reset" successful or not, "has_target" can be used 
   * (called right after "reset").
   */
  template <typename Handler>
  void reset(const Handler& handler)
  {
    service_.reset(implementation_, handler);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

  /// Post the stored handler to storage related io_service instance.
  /**
   * Attention! 
   * Alwasy check if handler storage has any handler stored in it.
   * Use "has_target". Always - even if you already have called "put" method.
   * Really, "put" means "try to put, if can't (io_service's destructor is
   * already called) then do nothing".
   */
  void post(const arg_type& arg)
  {      
    service_.post(implementation_, arg);
  }    
    
private:
  /// The service associated with the storage.
  service_type& service_;

  /// The underlying implementation of the storage.
  implementation_type implementation_;
}; // class handler_storage  

} // namespace ma

#endif // MA_HANDLER_STORAGE_HPP
