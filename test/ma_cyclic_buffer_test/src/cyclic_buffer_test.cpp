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

TEST(zero_size_cyclic_buffer, no_free_space)
{
  typedef ma::cyclic_buffer::mutable_buffers_type mutable_buffers_type;
  ma::cyclic_buffer test_buffer(0);
  mutable_buffers_type free_space = test_buffer.prepared();
  ASSERT_EQ(0U, boost::asio::buffer_size(free_space));
}

TEST(zero_size_cyclic_buffer, no_filled_space)
{
  typedef ma::cyclic_buffer::const_buffers_type const_buffers_type;
  ma::cyclic_buffer test_buffer(0);
  const_buffers_type filled_space = test_buffer.data();
  ASSERT_EQ(0U, boost::asio::buffer_size(filled_space));
}

} // namespace cyclic_buffer_test
} // namespace test
} // namespace ma
