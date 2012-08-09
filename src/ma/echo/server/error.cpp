//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/server/error.hpp>

namespace ma {
namespace echo {
namespace server {

namespace {

class server_error_category_impl : public boost::system::error_category
{
public:
  server_error_category_impl()
  {
  }

  virtual const char* name() const
  {
    return "ma::echo::server";
  }

  virtual std::string message(int ev) const
  {
    switch (ev)
    {
    case server_error::invalid_state:
      return "Invalid state";
    case server_error::operation_aborted:
      return "Operation aborted";
    case server_error::inactivity_timeout:
      return "Inactivity timeout";
    case server_error::no_memory:
      return "No memory";
    case server_error::out_of_work:
      return "Run out of work";
    default:
      return "Unknown ma::echo::server error";
    }
  }

}; // class server_error_category_impl

} // anonymous namespace

const boost::system::error_category& server_error_category()
{
  static const server_error_category_impl error_category_const;
  return error_category_const;
}

} // namespace server
} // namespace echo
} // namespace ma
