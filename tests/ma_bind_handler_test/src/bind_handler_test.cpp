//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <string>
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/bind_handler.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {
namespace bind_handler {

typedef int arg1_type;
typedef const int* arg2_type;
typedef std::string arg3_type;

static const arg1_type arg1_value = 42;
static const arg2_type arg2_value = detail::addressof(arg1_value);
static const arg3_type arg3_value = "42";

class tracking_handler
{
private:
  typedef tracking_handler this_type;

  MA_DELETED_COPY_ASSIGNMENT_OPERATOR(this_type)

public:
  tracking_handler(detail::latch& alloc_counter, detail::latch& dealloc_counter,
      detail::latch& invoke_counter)
    : alloc_counter_(alloc_counter)
    , dealloc_counter_(dealloc_counter)
    , invoke_counter_(invoke_counter)
  {
  }

  void operator()(arg1_type& arg1)
  {
    arg1 = arg1_value;
  }

  void operator()(arg1_type& arg1, arg2_type& arg2)
  {
    arg1 = arg1_value;
    arg2 = arg2_value;
  }

  void operator()(arg1_type& arg1, arg2_type& arg2, arg3_type& arg3)
  {
    arg1 = arg1_value;
    arg2 = arg2_value;
    arg3 = arg3_value;
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
}; // class tracking_handler

TEST(bind_handler, delegation_with_1_arg)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  arg1_type arg1 = 0;

  boost::asio::io_service io_service;
  io_service.post(ma::bind_handler(
      tracking_handler(alloc_counter, dealloc_counter, invoke_counter),
      detail::ref(arg1)));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(arg1_value, arg1);
}

TEST(bind_handler, delegation_with_2_args)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  arg1_type arg1 = 0;
  arg2_type arg2 = 0;

  boost::asio::io_service io_service;
  io_service.post(ma::bind_handler(
      tracking_handler(alloc_counter, dealloc_counter, invoke_counter),
      detail::ref(arg1), detail::ref(arg2)));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(arg1_value, arg1);
  ASSERT_EQ(arg2_value, arg2);
}

TEST(bind_handler, delegation_with_3_args)
{
  detail::latch alloc_counter;
  detail::latch dealloc_counter;
  detail::latch invoke_counter;
  arg1_type arg1 = 0;
  arg2_type arg2 = 0;
  arg3_type arg3;

  boost::asio::io_service io_service;
  io_service.post(ma::bind_handler(
      tracking_handler(alloc_counter, dealloc_counter, invoke_counter),
      detail::ref(arg1), detail::ref(arg2), detail::ref(arg3)));
  io_service.run();

  ASSERT_LE(1U, alloc_counter.value());
  ASSERT_LE(1U, dealloc_counter.value());
  ASSERT_EQ(alloc_counter.value(), dealloc_counter.value());
  ASSERT_EQ(1U, invoke_counter.value());
  ASSERT_EQ(arg1_value, arg1);
  ASSERT_EQ(arg2_value, arg2);
  ASSERT_EQ(arg3_value, arg3);
}

} // namespace bind_handler
} // namespace test
} // namespace ma
