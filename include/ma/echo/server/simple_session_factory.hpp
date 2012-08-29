//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SIOIOS_SESSION_FACTORY_HPP
#define MA_ECHO_SERVER_SIOIOS_SESSION_FACTORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <ma/sp_intrusive_list.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/session_factory.hpp>

namespace ma {
namespace echo {
namespace server {

class simple_session_factory
  : private boost::noncopyable
  , public session_factory
{
private:
  typedef simple_session_factory this_type;

public:
  simple_session_factory(boost::asio::io_service& io_service,
      std::size_t max_recycled)
    : max_recycled_(max_recycled)
    , io_service_(io_service)
  {
  }

  ~simple_session_factory()
  {
  }

  session_ptr create(const session_config&, boost::system::error_code&);
  void release(const session_ptr&);

private:
  struct  session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;

  struct session_wrapper
    : public session
    , public sp_intrusive_list<session_wrapper>::base_hook
  {
    session_wrapper(boost::asio::io_service& io_service,
        const session_config& config)
      : session(io_service, config)
    {
    }

#if !defined(NDEBUG)
    ~session_wrapper()
    {
    }
#endif
  }; // struct session_wrapper

  typedef sp_intrusive_list<session_wrapper> session_list;

  const std::size_t        max_recycled_;
  boost::asio::io_service& io_service_;
  session_list             recycled_;
}; // class simple_session_factory

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SIOIOS_SESSION_FACTORY_HPP
