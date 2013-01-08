//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_INTERFACE_HPP
#define MA_TUTORIAL_ASYNC_INTERFACE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace tutorial {

class async_interface;
typedef boost::shared_ptr<async_interface> async_interface_ptr;

class async_interface
{
private:
  typedef async_interface this_type;

public:

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  static void async_do_something(async_interface_ptr, Handler&& handler);

#else

  template <typename Handler>
  static void async_do_something(async_interface_ptr, Handler&& handler);

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else  // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  static void async_do_something(const async_interface_ptr&,
      const Handler& handler);

#endif // defined(MA_HAS_RVALUE_REFS)

protected:
  typedef boost::system::error_code            do_something_result;
  typedef boost::optional<do_something_result> optional_do_something_result;
  typedef ma::handler_storage<do_something_result>
      do_something_handler_storage_type;

  // No-op member functions
  async_interface();
  ~async_interface();
  async_interface(const this_type&);
  this_type& operator=(const this_type&);

  // Member functions that have to be implemented
  virtual boost::asio::io_service::strand& strand() = 0;
  virtual do_something_handler_storage_type& do_something_handler_storage() = 0;
  virtual optional_do_something_result start_do_something() = 0;

private:

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Arg>
  class forward_handler_binder;

#endif

  template <typename Handler>
  static void start_do_something(const async_interface_ptr&, const Handler&);
}; // async_interface

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

template <typename Arg>
class async_interface::forward_handler_binder
{
private:
  typedef forward_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (*function_type)(const async_interface_ptr& async_interface,
      const Arg&);

  template <typename AsyncInterfacePtr>
  forward_handler_binder(function_type, AsyncInterfacePtr&&);

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  forward_handler_binder(this_type&&);
  forward_handler_binder(const this_type&);

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  void operator()(const Arg&);

private:
  function_type       function_;
  async_interface_ptr async_interface_;
}; // class async_interface::forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

template <typename Handler>
void async_interface::async_do_something(
    async_interface_ptr async_interface, Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  typedef void (*func_type)(const async_interface_ptr& async_interface,
      const handler_type&);

  func_type func = &this_type::start_do_something<handler_type>;

  async_interface->strand().post(ma::make_explicit_context_alloc_handler(
      std::forward<Handler>(handler), forward_handler_binder<handler_type>(
          func, std::move(async_interface))));
}

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

template <typename Handler>
void async_interface::async_do_something(
    async_interface_ptr async_interface, Handler&& handler)
{
  typedef typename remove_cv_reference<Handler>::type handler_type;
  typedef void (*func_type)(const async_interface_ptr& async_interface,
      const handler_type&);

  func_type func = &this_type::start_do_something<handler_type>;

  async_interface->strand().post(ma::make_explicit_context_alloc_handler(
      std::forward<Handler>(handler),
      boost::bind(func, std::move(async_interface), _1)));
}

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else  // defined(MA_HAS_RVALUE_REFS)

template <typename Handler>
void async_interface::async_do_something(
    const async_interface_ptr& async_interface, const Handler& handler)
{
  typedef Handler handler_type;
  typedef void (*func_type)(const async_interface_ptr& async_interface,
      const handler_type&);

  func_type func = &this_type::start_do_something<handler_type>;

  async_interface->strand().post(ma::make_explicit_context_alloc_handler(
      handler, boost::bind(func, async_interface, _1)));
}

#endif // defined(MA_HAS_RVALUE_REFS)

inline async_interface::async_interface()
{
}

inline async_interface::~async_interface()
{
}

inline async_interface::async_interface(const this_type&)
{
}

inline async_interface::this_type& async_interface::operator=(const this_type&)
{
  return *this;
}

template <typename Handler>
void async_interface::start_do_something(
    const async_interface_ptr& async_interface, const Handler& handler)
{
  if (optional_do_something_result result =
      async_interface->start_do_something())
  {
    async_interface->strand().get_io_service().post(
        ma::detail::bind_handler(handler, *result));
  }
  else
  {
    async_interface->do_something_handler_storage().store(handler);
  }
}

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

template <typename Arg>
template <typename AsyncInterfacePtr>
async_interface::forward_handler_binder<Arg>::forward_handler_binder(
    function_type function, AsyncInterfacePtr&& async_interface)
  : function_(function)
  , async_interface_(std::forward<AsyncInterfacePtr>(async_interface))
{
}

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
async_interface::forward_handler_binder<Arg>::forward_handler_binder(
    this_type&& other)
  : function_(other.function_)
  , async_interface_(std::move(other.async_interface_))
{
}

template <typename Arg>
async_interface::forward_handler_binder<Arg>::forward_handler_binder(
    const this_type& other)
  : function_(other.function_)
  , async_interface_(other.async_interface_)
{
}

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

template <typename Arg>
void async_interface::forward_handler_binder<Arg>::operator()(const Arg& arg)
{
  (*function_)(async_interface_, arg);
}

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_INTERFACE_HPP
