//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <limits>
#include <boost/numeric/conversion/cast.hpp>
#include <gtest/gtest.h>
#include <ma/limited_int.hpp>

namespace ma {
namespace test {
namespace limited_int {

typedef std::size_t counter_value_type;
typedef ma::limited_int<counter_value_type> counter_type;

TEST(limited_int, initial_state)
{
  {
    const counter_value_type zero = boost::numeric_cast<counter_value_type>(0);
    counter_type counter;
    ASSERT_EQ(zero, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
  {
    const counter_value_type one = boost::numeric_cast<counter_value_type>(1);
    counter_type counter(one);
    ASSERT_EQ(one, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
  {
    const counter_value_type min_value =
        (std::numeric_limits<counter_value_type>::min)();
    counter_type counter(min_value);
    ASSERT_EQ(min_value, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
  {
    const counter_value_type max_value =
        (std::numeric_limits<counter_value_type>::max)();
    counter_type counter(max_value);
    ASSERT_EQ(max_value, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
}

TEST(limited_int, max_value)
{
  const counter_value_type expected_max =
      (std::numeric_limits<counter_value_type>::max)();
  const counter_value_type actual_max = (counter_type::max)();
  ASSERT_EQ(expected_max, actual_max);
}

TEST(limited_int, add_without_overflow)
{
  {
    const counter_value_type add = boost::numeric_cast<counter_value_type>(42);
    counter_type counter;
    counter += add;
    ASSERT_EQ(add, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
  {
    const counter_value_type one = boost::numeric_cast<counter_value_type>(1);
    counter_type counter;
    ++counter;
    ASSERT_EQ(one, counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
  {
    const counter_value_type add = boost::numeric_cast<counter_value_type>(42);
    counter_type add_counter(add);
    counter_type counter(add);
    counter += add_counter;
    ASSERT_EQ(boost::numeric_cast<counter_value_type>(add + add),
        counter.value());
    ASSERT_FALSE(counter.overflowed());
  }
}

TEST(limited_int, add_with_overflow)
{
  const counter_value_type max_value =
      (std::numeric_limits<counter_value_type>::max)();
  const counter_value_type one = boost::numeric_cast<counter_value_type>(1);
  {
    counter_type counter(max_value);
    ++counter;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(boost::numeric_cast<counter_value_type>(42));
    counter += max_value;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(one);
    counter += max_value;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(one);
    counter_type max_counter(max_value);
    counter += max_counter;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter;
    counter_type overflowed_counter(max_value);
    ++overflowed_counter;
    counter += overflowed_counter;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
}

TEST(limited_int, add_when_overflowed)
{
  const counter_value_type max_value =
      (std::numeric_limits<counter_value_type>::max)();
  {
    counter_type counter(max_value);
    counter += max_value;
    ++counter;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(max_value);
    counter += max_value;
    counter += max_value;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(max_value);
    ++counter;
    counter += boost::numeric_cast<counter_value_type>(42);
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
  {
    counter_type counter(max_value);
    ++counter;
    counter_type overflowed_counter(max_value);
    ++overflowed_counter;
    counter += overflowed_counter;
    ASSERT_EQ(max_value, counter.value());
    ASSERT_TRUE(counter.overflowed());
  }
}

} // namespace limited_int
} // namespace test
} // namespace ma
