//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/client1/error.hpp>

namespace ma {

namespace echo { 

namespace client1 {

namespace {

class client1_error_category_impl : public boost::system::error_category
{
public:
  client1_error_category_impl()
  {
  }

  virtual const char* name() const
  {
    return "ma::echo::client1";
  }

  virtual std::string message(int ev) const
  {
    switch (ev)
    {
    case client1_error::invalid_state:
      return "Invalid state.";
    case client1_error::operation_aborted:
      return "Operation aborted.";
    case client1_error::operation_not_supported:
      return "Operation not supported.";
    default:
      return "Unknown ma::echo::client1 error.";
    }
  }

}; // class client1_error_category_impl

} // anonymous namespace
       
const boost::system::error_category& client1_error_category()
{
  static const client1_error_category_impl error_category_const;
  return error_category_const;
}

} // namespace server
} // namespace echo
} // namespace ma
