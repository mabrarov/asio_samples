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
  class session_wrapper_base
    : public sp_intrusive_list<session_wrapper_base>::base_hook
  {
  }; // class session_wrapper_base
  typedef sp_intrusive_list<session_wrapper_base> session_list;
  class session_wrapper;
  typedef boost::shared_ptr<session_wrapper> session_wrapper_ptr;

  class pool_item;
  typedef boost::shared_ptr<pool_item> pool_item_ptr;
  typedef std::vector<pool_item_ptr>   pool;
  typedef pool::const_iterator         pool_link;

  static pool create_pool(const io_service_vector& io_services,
      std::size_t max_recycled);

  const pool pool_;
}; // class pooled_session_factory

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_POOLED_SESSION_FACTORY_HPP
