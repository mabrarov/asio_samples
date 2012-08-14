//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_INTERFACE_ADAPTER_HPP
#define MA_TUTORIAL_ASYNC_INTERFACE_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <ma/config.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/tutorial/async_interface.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {
namespace tutorial {

class async_interface_adapter;
typedef boost::shared_ptr<async_interface_adapter> async_interface_adapter_ptr;

class async_interface_adapter
  : private boost::noncopyable
  , public boost::enable_shared_from_this<async_interface_adapter>
{
private:
  typedef async_interface_adapter this_type;

public:
  static async_interface_adapter_ptr create(const async_interface_ptr& ptr)
  {
    typedef shared_ptr_factory_helper<this_type> helper;
    return boost::make_shared<helper>(ptr);
  }

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_do_something(Handler&& handler)
  {
    typedef typename remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_do_something<handler_type>;

    strand_.post(ma::make_explicit_context_alloc_handler(
        std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_do_something(Handler&& handler)
  {
    typedef typename remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_do_something<handler_type>;

    strand_.post(ma::make_explicit_context_alloc_handler(
        std::forward<Handler>(handler),
        boost::bind(func, shared_from_this(), _1)));
  }

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else  // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_do_something(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_do_something<handler_type>;

    strand_.post(ma::make_explicit_context_alloc_handler(handler,
        boost::bind(func, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

protected:
  explicit async_interface_adapter(const async_interface_ptr& ptr)
    : strand_(ptr->get_strand())
    , do_something_handler_(ptr->get_do_something_handler_storage())
    , async_interface_(ptr)
  {
  }

  ~async_interface_adapter()
  {
  }

private:
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;
    typedef void (async_interface_adapter::*function_type)(const Arg&);

    template <typename AsyncInterfaceAdapterPtr>
    forward_handler_binder(function_type function,
        AsyncInterfaceAdapterPtr&& ptr)
      : function_(function)
      , async_interface_adapter_(std::forward<AsyncInterfaceAdapterPtr>(ptr))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

    forward_handler_binder(this_type&& other)
      : function_(other.function_)
      , async_interface_adapter_(std::move(other.async_interface_adapter_))
    {
    }

    forward_handler_binder(const this_type& other)
      : function_(other.function_)
      , async_interface_adapter_(other.async_interface_adapter_)
    {
    }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    void operator()(const Arg& arg)
    {
      ((*async_interface_adapter_).*function_)(arg);
    }

  private:
    function_type  function_;
    async_interface_adapter_ptr async_interface_adapter_;
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS)
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void start_do_something(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result =
        async_interface_->start_do_something())
    {
      strand_.get_io_service().post(ma::detail::bind_handler(
          handler, *result));
    }
    else
    {
      do_something_handler_.store(handler);
    }
  }

  boost::asio::io_service::strand& strand_;
  async_interface::do_something_handler_storage_type& do_something_handler_;
  async_interface_ptr async_interface_;
}; // class async_interface_adapter

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_INTERFACE_ADAPTER_HPP
