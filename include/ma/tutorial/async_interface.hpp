//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
  static void async_do_something(async_interface_ptr async_interface, 
      Handler&& handler)
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
  static void async_do_something(async_interface_ptr async_interface, 
      Handler&& handler)
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
  static void async_do_something(const async_interface_ptr& async_interface, 
      const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (*func_type)(const async_interface_ptr& async_interface,
        const handler_type&);

    func_type func = &this_type::start_do_something<handler_type>;

    async_interface->strand().post(ma::make_explicit_context_alloc_handler(
        handler, boost::bind(func, async_interface, _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

protected:
  async_interface()
  {
  }

  ~async_interface()
  {
  }

  async_interface(const this_type&)
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }

  typedef ma::handler_storage<boost::system::error_code>
      do_something_handler_storage_type;

  virtual boost::asio::io_service::strand& strand() = 0;

  virtual do_something_handler_storage_type&
  do_something_handler_storage() = 0;

  virtual boost::optional<boost::system::error_code>
  start_do_something() = 0;

private:
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;
    typedef void (*function_type)(const async_interface_ptr& async_interface,
        const Arg&);

    template <typename AsyncInterfacePtr>
    forward_handler_binder(function_type function,
        AsyncInterfacePtr&& async_interface)
      : function_(function)
      , async_interface_(std::forward<AsyncInterfacePtr>(async_interface))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

    forward_handler_binder(this_type&& other)
      : function_(other.function_)
      , async_interface_(std::move(other.async_interface_))
    {
    }

    forward_handler_binder(const this_type& other)
      : function_(other.function_)
      , async_interface_(other.async_interface_)
    {
    }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    void operator()(const Arg& arg)
    {
      (*function_)(async_interface_, arg);
    }

  private:
    function_type       function_;
    async_interface_ptr async_interface_;
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  static void start_do_something(const async_interface_ptr& async_interface,
      const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result =
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
}; // async_interface

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_INTERFACE_HPP
