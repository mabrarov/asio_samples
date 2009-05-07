//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_WORK_HANDLER_HPP
#define MA_WORK_HANDLER_HPP

#include <cstddef>
#include <boost/asio.hpp>
#include <ma/handler_alloc_helpers.hpp>
#include <ma/handler_invoke_helpers.hpp>

namespace ma
{    
  template <typename Handler>
  class work_handler
  {
  private:
    typedef work_handler<Handler> this_type;
    const this_type& operator=(const this_type&);

  public:
    typedef void result_type;

    work_handler(boost::asio::io_service& io_service, Handler handler)
      : io_service_(io_service)
      , work_(io_service)
      , handler_(handler)
    {
    }

    friend void* asio_handler_allocate(std::size_t size, this_type* context)
    {
      return ma_alloc_helpers::allocate(size, &context->handler_);
    }

    friend void asio_handler_deallocate(void* pointer, std::size_t size, this_type* context)
    {
      ma_alloc_helpers::deallocate(pointer, size, &context->handler_);
    }      

    void operator()()
    {
      io_service_.post(handler_);
    }

    template <typename Arg1>
    void operator()(const Arg1& arg1)
    {
      io_service_.post(boost::asio::detail::bind_handler(
        handler_, arg1));
    }

    template <typename Arg1, typename Arg2>
    void operator()(const Arg1& arg1, const Arg2& arg2)
    {
      io_service_.post(boost::asio::detail::bind_handler(
        handler_, arg1, arg2));
    }

    template <typename Arg1, typename Arg2, typename Arg3>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
    {
      io_service_.post(boost::asio::detail::bind_handler(
        handler_, arg1, arg2, arg3));
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    {
      io_service_.post(boost::asio::detail::bind_handler(
        handler_, arg1, arg2, arg3, arg4));
    }

    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    void operator()(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    {
      io_service_.post(boost::asio::detail::bind_handler(
        handler_, arg1, arg2, arg3, arg4, arg5));
    }    

  private:
    boost::asio::io_service& io_service_;
    boost::asio::io_service::work work_;
    Handler handler_;
  }; // class work_handler   

  template <typename Handler>
  const work_handler<Handler> make_work_handler(boost::asio::io_service& io_service, Handler handler)
  {
    return work_handler<Handler>(io_service, handler);
  }
  
} // namespace ma

#endif // MA_WORK_HANDLER_HPP