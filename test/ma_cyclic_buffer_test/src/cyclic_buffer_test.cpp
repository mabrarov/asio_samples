//
// Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <iterator>
#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <ma/cyclic_buffer.hpp>

namespace ma {
namespace test {
namespace cyclic_buffer_test {

class generic_test : public testing::TestWithParam<std::size_t>
{
}; // class generic_test

INSTANTIATE_TEST_CASE_P(buffer_size, generic_test,
    testing::Values(0, 1, 2, 3, 5, 128));

TEST_P(generic_test, size)
{
  const std::size_t buffer_size = GetParam();
  const ma::cyclic_buffer buffer(buffer_size);
  ASSERT_EQ(buffer_size, buffer.size());
}

TEST_P(generic_test, free_size_of_empty)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  const std::size_t buffer_size = GetParam();
  const ma::cyclic_buffer buffer(buffer_size);
  const mutable_buffers_type free_space = buffer.prepared();
  ASSERT_EQ(buffer_size, boost::asio::buffer_size(free_space));
}

TEST_P(generic_test, filled_size_of_empty)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  const std::size_t buffer_size = GetParam();
  const ma::cyclic_buffer buffer(buffer_size);
  const const_buffers_type filled_space = buffer.data();
  ASSERT_EQ(0U, boost::asio::buffer_size(filled_space));
}

TEST_P(generic_test, free_size_of_full)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  const std::size_t buffer_size = GetParam();
  ma::cyclic_buffer buffer(buffer_size);
  buffer.consume(buffer_size);
  const mutable_buffers_type free_space = buffer.prepared();
  ASSERT_EQ(0U, boost::asio::buffer_size(free_space));
}

TEST_P(generic_test, filled_size_of_full)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  const std::size_t buffer_size = GetParam();
  ma::cyclic_buffer buffer(buffer_size);
  buffer.consume(buffer_size);
  const const_buffers_type filled_space = buffer.data();
  ASSERT_EQ(buffer_size, boost::asio::buffer_size(filled_space));
}

TEST_P(generic_test, filled_size_limited)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  const std::size_t buffer_size = GetParam();
  const std::size_t size_limit = buffer_size / 2;
  ma::cyclic_buffer buffer(buffer_size);
  buffer.consume(buffer_size);
  const const_buffers_type filled_space = buffer.data(size_limit);
  ASSERT_EQ(size_limit, boost::asio::buffer_size(filled_space));
}

TEST_P(generic_test, filled_size_limited_with_too_large_value)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  const std::size_t buffer_size = GetParam();
  const std::size_t size_limit = buffer_size + 1;
  ma::cyclic_buffer buffer(buffer_size);
  buffer.consume(buffer_size);
  const const_buffers_type filled_space = buffer.data(size_limit);
  ASSERT_EQ(buffer_size, boost::asio::buffer_size(filled_space));
}

TEST_P(generic_test, free_size_limited)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  const std::size_t buffer_size = GetParam();
  const std::size_t size_limit = buffer_size / 2;
  const ma::cyclic_buffer buffer(buffer_size);
  const mutable_buffers_type free_space = buffer.prepared(size_limit);
  ASSERT_EQ(size_limit, boost::asio::buffer_size(free_space));
}

TEST_P(generic_test, free_size_limited_with_too_large_value)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  const std::size_t buffer_size = GetParam();
  const std::size_t size_limit = buffer_size + 1;
  const ma::cyclic_buffer buffer(buffer_size);
  const mutable_buffers_type free_space = buffer.prepared(size_limit);
  ASSERT_EQ(buffer_size, boost::asio::buffer_size(free_space));
}

TEST(complex_test, looping_free_space)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  ma::cyclic_buffer buffer(16);
  buffer.consume(8);
  buffer.commit(4);
  const mutable_buffers_type free_space = buffer.prepared();
  // 2 buffers should be provided
  ASSERT_EQ(2U, std::distance(free_space.begin(), free_space.end()));
  // Check the size of free space
  ASSERT_EQ(12U, boost::asio::buffer_size(free_space));
}

TEST(complex_test, looping_filled_space)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  ma::cyclic_buffer buffer(16);
  buffer.consume(buffer.size());
  buffer.commit(12);
  buffer.consume(4);
  const const_buffers_type filled_space = buffer.data();
  // 2 buffers should be provided
  ASSERT_EQ(2U, std::distance(filled_space.begin(), filled_space.end()));
  // Check the size of free space
  ASSERT_EQ(8U, boost::asio::buffer_size(filled_space));
}

} // namespace cyclic_buffer_test
} // namespace test
} // namespace ma
