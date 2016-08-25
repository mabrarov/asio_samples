//
// Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <ma/cyclic_buffer.hpp>

namespace ma {
namespace test {
namespace cyclic_buffer_test {

namespace {

void test_free_size_of_empty(ma::cyclic_buffer& buffer)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  const mutable_buffers_type free_space = buffer.prepared();
  ASSERT_EQ(buffer.size(), boost::asio::buffer_size(free_space));
}

void test_filled_size_of_empty(const ma::cyclic_buffer& buffer)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  const const_buffers_type filled_space = buffer.data();
  ASSERT_EQ(0U, boost::asio::buffer_size(filled_space));
}

} // anonymous namespace

TEST(zero_size_cyclic_buffer, no_free_space)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  ma::cyclic_buffer test_buffer(0);
  const mutable_buffers_type free_space = test_buffer.prepared();
  ASSERT_EQ(0U, boost::asio::buffer_size(free_space));
}

TEST(zero_size_cyclic_buffer, no_filled_space)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  ma::cyclic_buffer test_buffer(0);
  const const_buffers_type filled_space = test_buffer.data();
  ASSERT_EQ(0U, boost::asio::buffer_size(filled_space));
}

TEST(one_size_cyclic_buffer, free_size_of_empty)
{
  ma::cyclic_buffer test_buffer(1);
  test_free_size_of_empty(test_buffer);
}

TEST(one_size_cyclic_buffer, filled_size_of_empty)
{
  ma::cyclic_buffer test_buffer(1);
  test_filled_size_of_empty(test_buffer);
}

TEST(generic_size_cyclic_buffer, free_size_of_empty)
{
  ma::cyclic_buffer test_buffer(128);
  test_free_size_of_empty(test_buffer);
}

TEST(generic_size_cyclic_buffer, filled_size_of_empty)
{
  ma::cyclic_buffer test_buffer(256);
  test_filled_size_of_empty(test_buffer);
}

} // namespace cyclic_buffer_test
} // namespace test
} // namespace ma
