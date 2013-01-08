//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_FACTORY_HPP
#define MA_ECHO_SERVER_SESSION_FACTORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_config_fwd.hpp>
#include <ma/echo/server/session_factory_fwd.hpp>

namespace ma {
namespace echo {
namespace server {

class session_factory
{
private:
  typedef session_factory this_type;

public:
  virtual session_ptr create(const session_config& config,
      boost::system::error_code& error) = 0;
  virtual void release(const session_ptr& session) = 0;

protected:
  session_factory()
  {
  }

  ~session_factory()
  {
  }

  session_factory(const this_type&)
  {
  }

  this_type& operator=(const this_type&)
  {
    return *this;
  }
}; // class session_factory

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_FACTORY_HPP
