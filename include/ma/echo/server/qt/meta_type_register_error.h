//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_META_TYPE_REGISTER_ERROR_H
#define MA_ECHO_SERVER_QT_META_TYPE_REGISTER_ERROR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <stdexcept>

namespace ma {

namespace echo {

namespace server {

namespace qt {

class meta_type_register_error : public std::runtime_error
{
public:
  explicit meta_type_register_error(const char* type_name)
    : std::runtime_error("failed to register meta type")
    , type_name_(type_name)
  {
  }

  const char* failed_type_name()
  {
    return type_name_;
  }

private:
  const char* type_name_;
}; // class meta_type_register_error

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_META_TYPE_REGISTER_ERROR_H
