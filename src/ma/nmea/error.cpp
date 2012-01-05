//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/nmea/error.hpp>

namespace ma {
namespace nmea {

namespace {

class session_error_category_impl : public boost::system::error_category
{
public:
  session_error_category_impl()
  {
  }

  virtual const char* name() const
  {
    return "ma::nmea::session";
  }

  virtual std::string message(int ev) const
  {
    switch (ev)
    {
    case session_error::invalid_state:
      return "Invalid state.";
    case session_error::operation_aborted:
      return "Operation aborted.";
    default:
      return "Unknown ma::echo::server error.";
    }
  }

}; // class session_error_category_impl

} // anonymous namespace

const boost::system::error_category& session_error_category()
{
  static const session_error_category_impl error_category_const;
  return error_category_const;
}

} // namespace nmea
} // namespace ma
