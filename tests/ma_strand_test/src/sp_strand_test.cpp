//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/strand.hpp>
#include <ma/detail/utility.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {
namespace strand {

typedef boost::optional<boost::asio::io_service::work> optional_work;

void save_thread_id(detail::thread::id& thread_id, detail::latch& done)
{
  thread_id = detail::this_thread::get_id();
  done.count_down();
}

template <typename Handler>
class dispatcher
{
public:
  dispatcher(boost::asio::io_service& io_service, MA_FWD_REF(Handler) handler)
    : io_service_(io_service)
    , handler_(detail::forward<Handler>(handler))
  {
  }

  void operator()()
  {
    io_service_.dispatch(detail::move(handler_));
  }

private:
  boost::asio::io_service& io_service_;
  Handler handler_;
}; // class dispatcher

template<typename Function>
dispatcher<typename detail::decay<Function>::type> make_dispatcher(
    boost::asio::io_service& io_service, MA_FWD_REF(Function) function)
{
  typedef typename detail::decay<Function>::type handler_type;
  return dispatcher<handler_type>(io_service,
      detail::forward<Function>(function));
}

TEST(strand, dispatch_in_same_io_service)
{
  typedef std::size_t (boost::asio::io_service::*run_io_service_func)(void);
  run_io_service_func run = &boost::asio::io_service::run;

  detail::latch done(1);
  detail::thread::id strand_thread_id;

  boost::asio::io_service io_service1;
  ma::strand strand(io_service1);
  boost::asio::io_service io_service2;

  optional_work work1(boost::in_place(detail::ref(io_service1)));
  optional_work work2(boost::in_place(detail::ref(io_service2)));

  detail::thread thread1(detail::bind(run, &io_service1));
  detail::thread thread2(detail::bind(run, &io_service2));

  io_service2.post(make_dispatcher(io_service1, detail::bind(
      save_thread_id, detail::ref(strand_thread_id), detail::ref(done))));

  done.wait();
  ASSERT_EQ(thread1.get_id(), strand_thread_id);

  work2 = boost::none;
  work1 = boost::none;
  thread2.join();
  thread1.join();
}


} // namespace strand
} // namespace test
} // namespace ma