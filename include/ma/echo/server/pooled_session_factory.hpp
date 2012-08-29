//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_POOLED_SESSION_FACTORY_HPP
#define MA_ECHO_SERVER_POOLED_SESSION_FACTORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <vector>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <ma/sp_intrusive_list.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/session_factory.hpp>

namespace ma {
namespace echo {
namespace server {

class pooled_session_factory
  : private boost::noncopyable
  , public session_factory
{
private:
  typedef pooled_session_factory this_type;

public:
  typedef std::vector<boost::shared_ptr<boost::asio::io_service> >
      io_service_vector;

  pooled_session_factory(const io_service_vector& io_services,
      std::size_t max_recycled);

  ~pooled_session_factory()
  {
  }

  session_ptr create(const session_config&, boost::system::error_code&);
  void release(const session_ptr&);

private:
  struct  io_service_pool_item;
  typedef boost::shared_ptr<io_service_pool_item> io_service_pool_item_ptr;
  typedef std::vector<io_service_pool_item_ptr> io_service_pool;

  struct  session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;

  struct session_wrapper
    : public session
    , public sp_intrusive_list<session_wrapper>::base_hook
  {
    typedef io_service_pool::const_iterator pool_link;

    session_wrapper(boost::asio::io_service& io_service,
        const session_config& config, const pool_link& the_back_link)
      : session(io_service, config)
      , back_link(the_back_link)
    {
    }

#if !defined(NDEBUG)
    ~session_wrapper()
    {
    }
#endif

    pool_link back_link;
  }; // struct session_wrapper

  typedef sp_intrusive_list<session_wrapper> session_list;

  struct io_service_pool_item
  {
    io_service_pool_item(boost::asio::io_service& the_io_service,
        std::size_t the_max_recycled)
      : max_recycled(the_max_recycled)
      , io_service(the_io_service)
    {
    }

    const std::size_t        max_recycled;
    boost::asio::io_service& io_service;
    session_list             recycled;
  }; // struct io_service_pool_item

  io_service_pool pool_;
  io_service_pool::const_iterator current_;
}; // class pooled_session_factory

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_POOLED_SESSION_FACTORY_HPP
