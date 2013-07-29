//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_DETAIL_SERVICE_BASE_HPP
#define MA_DETAIL_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>

namespace ma {
namespace detail {

// Special derived service id type to keep classes header-file only.
// Copied from boost/asio/io_service.hpp.
template <typename Type>
class service_id : public boost::asio::io_service::id
{
}; // class service_id

// Special service base class to keep classes header-file only.
// Copied from boost/asio/io_service.hpp.
template <typename Type>
class service_base : public boost::asio::io_service::service
{
public:
  static service_id<Type> id;

  explicit service_base(boost::asio::io_service& io_service);

protected:
  virtual ~service_base();
}; // class service_base

template <typename Type>
service_base<Type>::service_base(boost::asio::io_service& io_service)
  : boost::asio::io_service::service(io_service)
{
}

template <typename Type>
service_base<Type>::~service_base()
{
}

template <typename Type>
service_id<Type> service_base<Type>::id;

} // namespace detail
} // namespace ma

#endif // MA_DETAIL_SERVICE_BASE_HPP
