//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>
#include <ma/sp_intrusive_list.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/latch.hpp>

namespace ma {
namespace test {
namespace sp_intrusive_list {

class list_item
  : private boost::noncopyable
  , private ma::sp_intrusive_list<list_item>::base_hook
{
private:
  typedef list_item this_type;
  typedef ma::sp_intrusive_list<list_item> list_type;

  friend list_type;

public:
  explicit list_item(detail::latch& counter) : counter_(counter)
  {
  }

  ~list_item()
  {
    counter_.count_down();
  }

private:
  detail::latch& counter_;
}; // class list_item

typedef ma::sp_intrusive_list<list_item> list_type;
typedef detail::shared_ptr<list_item> list_item_ptr;

TEST(sp_intrusive_list, lifetime)
{
  detail::latch instance_counter(1);
  {
    list_type list;
    {
      list_item_ptr item = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item);
    }
    ASSERT_EQ(1U, instance_counter.value());
  }
  ASSERT_EQ(0U, instance_counter.value());
}

} // namespace sp_intrusive_list
} // namespace test
} // namespace ma