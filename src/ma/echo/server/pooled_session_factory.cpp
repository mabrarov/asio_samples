//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <new>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/pooled_session_factory.hpp>

namespace ma {
namespace echo {
namespace server {

pooled_session_factory::pooled_session_factory(
    const io_service_vector& io_services, std::size_t max_recycled)
  : pool_()
  , current_()
{
  for (io_service_vector::const_iterator i = io_services.begin(),
      end = io_services.end(); i != end; ++i)
  {
    pool_.push_back(boost::make_shared<io_service_pool_item>(
        boost::ref(**i), max_recycled));
  }
  current_ = pool_.begin();
}

session_ptr pooled_session_factory::create(const session_config& config,
    boost::system::error_code& error)
{
  session_wrapper_ptr session;

  io_service_pool_item& pool_item = **current_;
  if (pool_item.recycled.empty())
  {
    try
    {
      session = boost::make_shared<session_wrapper>(
          boost::ref(pool_item.io_service), config, current_);
      error = boost::system::error_code();
    }
    catch (const std::bad_alloc&)
    {
      error = server_error::no_memory;
    }
  }
  else
  {
    session = pool_item.recycled.front();
    pool_item.recycled.erase(session);
    error = boost::system::error_code();
  }

  ++current_;
  if (current_ == pool_.end())
  {
    current_ = pool_.begin();
  }

  return session;
}

void pooled_session_factory::release(const session_ptr& session)
{
  session_wrapper_ptr wrapped_session =
      boost::static_pointer_cast<session_wrapper>(session);
  io_service_pool_item& pool_item = **(wrapped_session->back_link);
  if (pool_item.max_recycled > pool_item.recycled.size())
  {
    pool_item.recycled.push_front(wrapped_session);
  }
}

} // namespace server
} // namespace echo
} // namespace ma
