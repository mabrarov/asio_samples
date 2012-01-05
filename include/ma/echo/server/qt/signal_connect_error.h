//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SIGNAL_CONNECT_ERROR_H
#define MA_ECHO_SERVER_QT_SIGNAL_CONNECT_ERROR_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace ma {
namespace echo {
namespace server {
namespace qt {

class signal_connect_error : public std::runtime_error
{
public:
  signal_connect_error()
    : std::runtime_error("failed to connect Qt signal")
  {
  }
}; // class signal_connect_error

inline void checkConnect(bool connectResult)
{
  if (!connectResult)
  {
    boost::throw_exception(signal_connect_error());
  }
}

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SIGNAL_CONNECT_ERROR_H
