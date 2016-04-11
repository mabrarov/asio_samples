//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TEST_IO_SERVICE_POOL_HPP
#define MA_TEST_IO_SERVICE_POOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <ma/thread_group.hpp>

namespace ma {
namespace test {

class io_service_pool : private boost::noncopyable
{
public:
  io_service_pool(boost::asio::io_service& io_service, std::size_t size);
  ~io_service_pool();

private:
  typedef boost::optional<boost::asio::io_service::work> optional_work;

  optional_work    work_;
  ma::thread_group threads_;
}; // class io_service_pool

} // namespace test
} // namespace ma

#endif // MA_TEST_IO_SERVICE_POOL_HPP
