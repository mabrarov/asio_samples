//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <new>
#include <algorithm>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/pooled_session_factory.hpp>

namespace ma {
namespace echo {
namespace server {

class pooled_session_factory::session_wrapper
  : public session_wrapper_base
  , public session
{
private:
  typedef session_wrapper this_type;

public:
  static session_wrapper_ptr create(boost::asio::io_service& io_service,
      const session_config& config, const pool_link& back_link)
  {
    typedef shared_ptr_factory_helper<this_type> helper;
    return boost::make_shared<helper>(
        boost::ref(io_service), config, back_link);
  }

  const pool_link& back_link() const
  {
    return back_link_;
  }

protected:
  session_wrapper(boost::asio::io_service& io_service,
      const session_config& config, const pool_link& back_link)
    : session(io_service, config)
    , back_link_(back_link)
  {
  }

  ~session_wrapper()
  {
  }

private:
  pool_link back_link_;
}; // class pooled_session_factory::session_wrapper

class pooled_session_factory::pool_item
{
public:
  pool_item(boost::asio::io_service& io_service, std::size_t max_recycled)
    : max_recycled_(max_recycled)
    , io_service_(io_service)
    , size_(0)
  {
  }

  session_wrapper_ptr create(const pool_link& back_link,
      const session_config& config, boost::system::error_code& error)
  {
    if (!recycled_.empty())
    {
      session_wrapper_ptr session = 
          boost::static_pointer_cast<session_wrapper>(recycled_.front());
      recycled_.erase(session);
      ++size_;
      error = boost::system::error_code();
      return session;
    }

    try
    {
      session_wrapper_ptr session = session_wrapper::create(
          io_service_, config, back_link);
      ++size_;
      error = boost::system::error_code();
      return session;
    }
    catch (const std::bad_alloc&)
    {
      error = server_error::no_memory;
      return session_wrapper_ptr();
    }
  }

  void release(const session_wrapper_ptr& session)
  {
    --size_;
    if (max_recycled_ > recycled_.size())
    {
      recycled_.push_front(session);
    }
  }

  static bool less_loaded_pool(const pool_item_ptr& left,
      const pool_item_ptr& right)
  {
    return left->size_ < right->size_;
  }

private:
  const std::size_t        max_recycled_;
  boost::asio::io_service& io_service_;
  std::size_t              size_;
  session_list             recycled_;
}; // class pooled_session_factory::pool_item

pooled_session_factory::pooled_session_factory(
    const io_service_vector& io_services, std::size_t max_recycled)
  : pool_(create_pool(io_services, max_recycled))
{
}

session_ptr pooled_session_factory::create(const session_config& config,
    boost::system::error_code& error)
{
  // Select appropriate item of pool
  const pool::const_iterator selected_pool_item =
      std::min_element(pool_.begin(), pool_.end(), pool_item::less_loaded_pool);

  // Create new session by means of selected pool item
  return (*selected_pool_item)->create(selected_pool_item, config, error);
}

void pooled_session_factory::release(const session_ptr& session)
{
  // Find session's pool item
  const session_wrapper_ptr wrapped_session =
      boost::static_pointer_cast<session_wrapper>(session);
  pool_item& session_pool_item = **(wrapped_session->back_link());
  // Release session by means of its pool item
  session_pool_item.release(wrapped_session);
}

pooled_session_factory::pool pooled_session_factory::create_pool(
    const io_service_vector& io_services, std::size_t max_recycled)
{
  pool result;
  for (io_service_vector::const_iterator i = io_services.begin(),
      end = io_services.end(); i != end; ++i)
  {
    result.push_back(boost::make_shared<pool_item>(
        boost::ref(**i), max_recycled));
  }
  return result;
}

} // namespace server
} // namespace echo
} // namespace ma
