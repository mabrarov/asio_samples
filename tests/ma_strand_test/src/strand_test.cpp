//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/strand.hpp>
#include <ma/io_context_helpers.hpp>
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
private:
  typedef dispatcher<Context, Handler> this_type;

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

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
private:
  typedef poster<Context, Handler> this_type;

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

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

class tracking_handler
{
private:
  typedef tracking_handler this_type;

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

public:
  tracking_handler(detail::latch& alloc_counter, detail::latch& dealloc_counter,
      detail::latch& invoke_counter, detail::latch& call_counter)
    : alloc_counter_(alloc_counter)
    , dealloc_counter_(dealloc_counter)
    , invoke_counter_(invoke_counter)
    , call_counter_(call_counter)
  {
  }

  void operator()()
  {
    call_counter_.count_up();
  }

  friend void* asio_handler_allocate(std::size_t size, this_type* context)
  {
    context->alloc_counter_.count_up();
    return boost::asio::asio_handler_allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      this_type* context)
  {
    context->dealloc_counter_.count_up();
    boost::asio::asio_handler_deallocate(pointer, size);
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(MA_FWD_REF(Function) function,
      this_type* context)
  {
    context->invoke_counter_.count_up();
    boost::asio::asio_handler_invoke(detail::forward<Function>(function));
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Function>
  friend void asio_handler_invoke(Function& function, this_type* context)
  {
    context->invoke_counter_.count_up();
    boost::asio::asio_handler_invoke(function);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function, this_type* context)
  {
    context->invoke_counter_.count_up();
    boost::asio::asio_handler_invoke(function);
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:
  detail::latch& alloc_counter_;
  detail::latch& dealloc_counter_;
  detail::latch& invoke_counter_;
  detail::latch& call_counter_;
}; // class tracking_handler

typedef std::size_t (boost::asio::io_service::*run_io_service_func)(void);
static const run_io_service_func run_io_service = &boost::asio::io_service::run;

TEST(strand, get_io_service)
{
  boost::asio::io_service io_service;
  ma::strand test_strand(io_service);
  boost::asio::io_service& strand_io_service = ma::get_io_context(test_strand);

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

TEST(strand, dispatch_delegation)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  detail::latch call_counter;

  boost::asio::io_service io_service;
  ma::strand test_strand(io_service);
  test_strand.dispatch(tracking_handler(
      alloc_counter, dealloc_counter, invoke_counter, call_counter));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(1U, call_counter.value());
}

TEST(strand, post_delegation)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  detail::latch call_counter;

  boost::asio::io_service io_service;
  ma::strand test_strand(io_service);
  test_strand.post(tracking_handler(
      alloc_counter, dealloc_counter, invoke_counter, call_counter));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(1U, call_counter.value());
}

TEST(strand, wrapped_delegation)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  detail::latch call_counter;

  boost::asio::io_service io_service;
  ma::strand test_strand(io_service);
  io_service.post(test_strand.wrap(tracking_handler(
      alloc_counter, dealloc_counter, invoke_counter, call_counter)));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(1U, call_counter.value());
}

} // namespace strand
} // namespace test
} // namespace ma
