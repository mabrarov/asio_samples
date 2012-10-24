//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/nmea/error.hpp>

namespace ma {
namespace nmea {

namespace {

class error_category_impl : public boost::system::error_category
{
public:
  error_category_impl()
  {
  }

  virtual const char* name() const
  {
    return "ma.nmea";
  }

  virtual std::string message(int ev) const
  {
    switch (ev)
    {
    case error::invalid_state:
      return "Invalid state";
    case error::operation_aborted:
      return "Operation aborted";
    default:
      return "ma.nmea error";
    }
  }

}; // class error_category_impl

} // anonymous namespace

const boost::system::error_category& error::category()
{
  static const error_category_impl instance;
  return instance;
}

} // namespace nmea
} // namespace ma
