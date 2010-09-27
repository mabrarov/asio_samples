//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL_ASYNC_BASE_HPP
#define MA_TUTORIAL_ASYNC_BASE_HPP

#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/bind_asio_handler.hpp>

namespace ma
{
  namespace tutorial
  {    
    class Async_base 
      : public boost::enable_shared_from_this<Async_base>
      , private boost::noncopyable
    {
    private:
      typedef Async_base this_type;

    public:
      template <typename Handler>
      void async_do_something(Handler handler)
      {
        strand_.dispatch(ma::make_context_alloc_handler2(handler, 
          boost::bind(&this_type::call_do_something<Handler>, shared_from_this(), _1)));
      }

    protected:
      Async_base(boost::asio::io_service::strand& strand)
        : strand_(strand)
        , do_something_handler_(strand.get_io_service())
      {
      }

      ~Async_base() 
      {
      }

      virtual boost::optional<boost::system::error_code> do_something() = 0;

      void complete_do_something(const boost::system::error_code& error)
      {
        do_something_handler_.post(error);
      }

      bool has_do_something_target() const
      {
        return do_something_handler_.has_target();
      }

    private:
      template <typename Handler>
      void call_do_something(const Handler& handler)
      {    
        if (boost::optional<boost::system::error_code> result = do_something())
        {
          strand_.get_io_service().post(ma::detail::bind_handler(handler, *result));
        }
        else
        {
          do_something_handler_.store(handler);
        }
      }

      boost::asio::io_service::strand& strand_;
      ma::handler_storage<boost::system::error_code> do_something_handler_;
    }; //class Async_base

  } // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_BASE_HPP
