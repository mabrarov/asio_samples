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
#include <ma/echo/server/simple_session_factory.hpp>

namespace ma {
namespace echo {
namespace server {

session_ptr simple_session_factory::create(const session_config& config,
    boost::system::error_code& error)
{
  if (!recycled_.empty())
  {
    session_wrapper_ptr session = recycled_.front();
    recycled_.erase(session);
    error = boost::system::error_code();
    return session;
  }

  try
  {
    session_wrapper_ptr session = boost::make_shared<session_wrapper>(
        boost::ref(io_service_), config);
    error = boost::system::error_code();
    return session;
  }
  catch (const std::bad_alloc&)
  {
    error = server_error::no_memory;
    return session_ptr();
  }
}

void simple_session_factory::release(const session_ptr& session)
{
  if (max_recycled_ > recycled_.size())
  {
    recycled_.push_front(boost::static_pointer_cast<session_wrapper>(session));
  }
}

} // namespace server
} // namespace echo
} // namespace ma
