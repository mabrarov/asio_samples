//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONDITION_HPP
#define MA_CONDITION_HPP

#include <cstddef>
#include <boost/utility.hpp>
#include <ma/condition_service.hpp>

namespace ma
{
  template <typename Arg>
  class condition : private boost::noncopyable
  {
  public:
    typedef Arg argument_type;
    typedef condition_service<argument_type> service_type;
    typedef typename service_type::implementation_type implementation_type;

    explicit condition(boost::asio::io_service& io_service)
      : service_(boost::asio::use_service<service_type>(io_service))
    {
      service_.construct(implementation_);
    }

    ~condition()
    {
      service_.destroy(implementation_);
    } 

    boost::asio::io_service& io_service()
    {
      return service_.get_io_service();
    }

    boost::asio::io_service& get_io_service()
    {
      return service_.get_io_service();
    }

    template <typename Handler>
    void async_wait(argument_type cancel_arg, Handler handler)
    {
      service_.async_wait(implementation_, cancel_arg, handler);
    }

    std::size_t fire_now(argument_type arg)
    {
      return service_.fire_now(implementation_, arg);
    }

    std::size_t cancel()
    {
      return service_.cancel(implementation_);
    }

    std::size_t get_charge()
    {
      return service_.get_charge(implementation_);
    }

  private:
    // The service associated with the I/O object.
    service_type& service_;

    // The underlying implementation of the I/O object.
    implementation_type implementation_;
  }; // class condition

} // namespace ma

#endif // MA_CONDITION_HPP