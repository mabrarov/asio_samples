//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
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
      std::size_t max_recycled);

#if !defined(NDEBUG)
  ~simple_session_factory();
#endif

  session_ptr create(const session_config& config,
      boost::system::error_code& error);
  void release(const session_ptr& session);

private:
  class session_wrapper_base
    : public sp_intrusive_list<session_wrapper_base>::base_hook
  {
  }; // class session_wrapper_base

  typedef sp_intrusive_list<session_wrapper_base> session_list;

  class session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;

  const std::size_t        max_recycled_;
  boost::asio::io_service& io_service_;
  session_list             recycled_;
}; // class simple_session_factory

inline simple_session_factory::simple_session_factory(
    boost::asio::io_service& io_service, std::size_t max_recycled)
  : max_recycled_(max_recycled)
  , io_service_(io_service)
{
}

#if !defined(NDEBUG)
inline simple_session_factory::~simple_session_factory()
{
}
#endif

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SIOIOS_SESSION_FACTORY_HPP
