//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_INTERFACE_HPP
#define MA_TUTORIAL_ASYNC_INTERFACE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/utility.hpp>

namespace ma {
namespace tutorial {

class async_interface;
typedef detail::shared_ptr<async_interface> async_interface_ptr;

class async_interface
{
private:
  typedef async_interface this_type;

public:

  template <typename Handler>
  void async_do_something(MA_FWD_REF(Handler));

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
  virtual async_interface_ptr get_interface_ptr() = 0;
  virtual boost::asio::io_service::strand& strand() = 0;
  virtual do_something_handler_storage_type& do_something_handler_storage() = 0;
  virtual optional_do_something_result start_do_something() = 0;

private:

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)

  template <typename Arg>
  class forward_handler_binder;

#endif

  template <typename Handler>
  void start_do_something(Handler&);
}; // async_interface

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)

template <typename Arg>
class async_interface::forward_handler_binder
{
private:
  typedef forward_handler_binder this_type;

public:
  typedef void result_type;
  typedef void (async_interface::*function_type)(Arg&);

  template <typename AsyncInterfacePtr>
  forward_handler_binder(function_type, AsyncInterfacePtr&&);

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  forward_handler_binder(this_type&&);
  forward_handler_binder(const this_type&);

#endif // defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR)

  void operator()(Arg&);

private:
  function_type       function_;
  async_interface_ptr async_interface_;
}; // class async_interface::forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)

template <typename Handler>
void async_interface::async_do_something(MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef void (async_interface::*func_type)(handler_type&);

  func_type func = &this_type::start_do_something<handler_type>;

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)
  strand().post(ma::make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      forward_handler_binder<handler_type>(func, get_interface_ptr())));
#else
  strand().post(ma::make_explicit_context_alloc_handler(
      detail::forward<Handler>(handler),
      detail::bind(func, get_interface_ptr(), detail::placeholders::_1)));
#endif
}

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
void async_interface::start_do_something(Handler& handler)
{
  if (optional_do_something_result result = start_do_something())
  {
    ma::get_io_context(strand()).post(
        ma::bind_handler(detail::move(handler), *result));
  }
  else
  {
    do_something_handler_storage().store(detail::move(handler));
  }
}

#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)

template <typename Arg>
template <typename AsyncInterfacePtr>
async_interface::forward_handler_binder<Arg>::forward_handler_binder(
    function_type function, AsyncInterfacePtr&& async_interface)
  : function_(function)
  , async_interface_(detail::forward<AsyncInterfacePtr>(async_interface))
{
}

#if defined(MA_NO_IMPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

template <typename Arg>
async_interface::forward_handler_binder<Arg>::forward_handler_binder(
    this_type&& other)
  : function_(other.function_)
  , async_interface_(detail::move(other.async_interface_))
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
void async_interface::forward_handler_binder<Arg>::operator()(Arg& arg)
{
  ((*async_interface_).*function_)(arg);
}

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BIND_HAS_NO_MOVE_CONSTRUCTOR)

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_INTERFACE_HPP
