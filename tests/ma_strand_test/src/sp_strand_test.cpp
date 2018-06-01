//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/strand.hpp>
#include <ma/detail/utility.hpp>
#include <ma/detail/memory.hpp>
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

template <typename Context, typename Handler>
class dispatcher
{
public:
  dispatcher(Context& context, MA_FWD_REF(Handler) handler)
    : context_(context)
    , handler_(detail::forward<Handler>(handler))
  {
  }

  void operator()()
  {
    context_.dispatch(detail::move(handler_));
  }

private:
  Context& context_;
  Handler handler_;
}; // class dispatcher

template<typename Context, typename Handler>
dispatcher<typename detail::decay<Context>::type,
    typename detail::decay<Handler>::type> make_dispatcher(
    Context& context, MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Context>::type context_type;
  typedef typename detail::decay<Handler>::type handler_type;
  return dispatcher<context_type, handler_type>(context,
      detail::forward<Handler>(handler));
}

template <typename Context, typename Handler>
class poster
{
public:
  poster(Context& context, MA_FWD_REF(Handler) handler)
    : context_(context)
    , handler_(detail::forward<Handler>(handler))
  {
  }

  void operator()()
  {
    context_.post(detail::move(handler_));
  }

private:
  Context& context_;
  Handler handler_;
}; // class poster

template<typename Context, typename Handler>
poster<typename detail::decay<Context>::type,
    typename detail::decay<Handler>::type> make_poster(
    Context& context, MA_FWD_REF(Handler) handler)
{
  typedef typename detail::decay<Context>::type context_type;
  typedef typename detail::decay<Handler>::type handler_type;
  return poster<context_type, handler_type>(context,
      detail::forward<Handler>(handler));
}

class io_service_thread_stop : private boost::noncopyable
{
public:
  io_service_thread_stop(optional_work& io_service_work, detail::thread& thread)
    : io_service_work_(io_service_work)
    , thread_(thread)
  {
  }

  ~io_service_thread_stop()
  {
    io_service_work_ = boost::none;
    thread_.join();
  }

private:
  optional_work& io_service_work_;
  detail::thread& thread_;
}; // class io_service_thread_stop

typedef std::size_t (boost::asio::io_service::*run_io_service_func)(void);
static const run_io_service_func run_io_service = &boost::asio::io_service::run;

TEST(strand, get_io_service)
{
  boost::asio::io_service io_service;
  ma::strand test_strand(io_service);
  boost::asio::io_service& strand_io_service = test_strand.get_io_service();

  ASSERT_EQ(detail::addressof(io_service),
      detail::addressof(strand_io_service));
}

TEST(strand, dispatch_same_io_service)
{
  detail::latch done(1);
  detail::thread::id handler_thread_id;

  boost::asio::io_service io_service1;
  ma::strand test_strand(io_service1);
  boost::asio::io_service io_service2;

  optional_work work1(boost::in_place(detail::ref(io_service1)));
  detail::thread thread1(detail::bind(run_io_service, &io_service1));
  io_service_thread_stop thread1_stop(work1, thread1);
  optional_work work2(boost::in_place(detail::ref(io_service2)));
  detail::thread thread2(detail::bind(run_io_service, &io_service2));
  io_service_thread_stop thread2_stop(work2, thread2);

  io_service2.post(make_dispatcher(test_strand, detail::bind(
      save_thread_id, detail::ref(handler_thread_id), detail::ref(done))));

  done.wait();
  ASSERT_EQ(thread1.get_id(), handler_thread_id);

  (void) thread2_stop;
  (void) thread1_stop;
}

TEST(strand, wrapped_dispatch_same_io_service)
{
  detail::latch done(1);
  detail::thread::id handler_thread_id;

  boost::asio::io_service io_service1;
  ma::strand test_strand(io_service1);
  boost::asio::io_service io_service2;

  optional_work work1(boost::in_place(detail::ref(io_service1)));
  detail::thread thread1(detail::bind(run_io_service, &io_service1));
  io_service_thread_stop thread1_stop(work1, thread1);
  optional_work work2(boost::in_place(detail::ref(io_service2)));
  detail::thread thread2(detail::bind(run_io_service, &io_service2));
  io_service_thread_stop thread2_stop(work2, thread2);

  io_service2.post(make_dispatcher(io_service2, test_strand.wrap(detail::bind(
      save_thread_id, detail::ref(handler_thread_id), detail::ref(done)))));

  done.wait();
  ASSERT_EQ(thread1.get_id(), handler_thread_id);

  (void) thread2_stop;
  (void) thread1_stop;
}

TEST(strand, post_same_io_service)
{
  detail::latch done(1);
  detail::thread::id handler_thread_id;

  boost::asio::io_service io_service1;
  ma::strand test_strand(io_service1);
  boost::asio::io_service io_service2;

  optional_work work1(boost::in_place(detail::ref(io_service1)));
  detail::thread thread1(detail::bind(run_io_service, &io_service1));
  io_service_thread_stop thread1_stop(work1, thread1);
  optional_work work2(boost::in_place(detail::ref(io_service2)));
  detail::thread thread2(detail::bind(run_io_service, &io_service2));
  io_service_thread_stop thread2_stop(work2, thread2);

  io_service2.post(make_poster(test_strand, detail::bind(
      save_thread_id, detail::ref(handler_thread_id), detail::ref(done))));

  done.wait();
  ASSERT_EQ(thread1.get_id(), handler_thread_id);

  (void) thread2_stop;
  (void) thread1_stop;
}

TEST(strand, wrapped_post_same_io_service)
{
  detail::latch done(1);
  detail::thread::id handler_thread_id;

  boost::asio::io_service io_service1;
  ma::strand test_strand(io_service1);
  boost::asio::io_service io_service2;

  optional_work work1(boost::in_place(detail::ref(io_service1)));
  detail::thread thread1(detail::bind(run_io_service, &io_service1));
  io_service_thread_stop thread1_stop(work1, thread1);
  optional_work work2(boost::in_place(detail::ref(io_service2)));
  detail::thread thread2(detail::bind(run_io_service, &io_service2));
  io_service_thread_stop thread2_stop(work2, thread2);

  io_service2.post(make_poster(io_service2, test_strand.wrap(detail::bind(
      save_thread_id, detail::ref(handler_thread_id), detail::ref(done)))));

  done.wait();
  ASSERT_EQ(thread1.get_id(), handler_thread_id);

  (void) thread2_stop;
  (void) thread1_stop;
}

} // namespace strand
} // namespace test
} // namespace ma