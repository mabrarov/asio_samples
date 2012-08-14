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
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <ma/handler_storage.hpp>

namespace ma {
namespace tutorial {

class async_interface;
typedef boost::shared_ptr<async_interface> async_interface_ptr;

class async_interface
{
private:
  typedef async_interface this_type;

public:
  typedef ma::handler_storage<boost::system::error_code>
      do_something_handler_storage_type;

  virtual async_interface_ptr shared_async_interface() = 0;

  virtual boost::asio::io_service::strand& strand() = 0;

  virtual do_something_handler_storage_type&
  do_something_handler_storage() = 0;

  virtual boost::optional<boost::system::error_code>
  start_do_something() = 0;

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
}; // async_interface

} // namespace tutorial
} // namespace ma

#endif // MA_TUTORIAL_ASYNC_INTERFACE_HPP
