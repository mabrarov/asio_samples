//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/utility/in_place_factory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/test/io_service_pool.hpp>

namespace ma {
namespace test {

io_service_pool::io_service_pool(boost::asio::io_service& io_service,
    std::size_t size)
  : work_(boost::in_place(detail::ref(io_service)))
{
  typedef std::size_t (boost::asio::io_service::*run_func)(void);
  run_func run = &boost::asio::io_service::run;

  for (std::size_t i = 0; i < size; ++i)
  {
    threads_.create_thread(detail::bind(run, &io_service));
  }
}

io_service_pool::~io_service_pool()
{
  boost::asio::io_service& io_service = work_->get_io_service();
  work_ = boost::none;
  io_service.stop();
  threads_.join_all();
}

} // namespace test
} // namespace ma
