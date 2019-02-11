//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <vector>
#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>
#include <ma/sp_intrusive_list.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/memory.hpp>

namespace ma {
namespace test {
namespace sp_intrusive_list {

class list_item
  : private boost::noncopyable
  , private ma::sp_intrusive_list<list_item>::base_hook
{
private:
  typedef list_item this_type;

  friend class ma::sp_intrusive_list<list_item>;

public:
  explicit list_item(int& counter) : counter_(counter)
  {
    ++counter_;
  }

  ~list_item()
  {
    --counter_;
  }

private:
  int& counter_;
}; // class list_item

typedef ma::sp_intrusive_list<list_item> list_type;
typedef detail::shared_ptr<list_item> list_item_ptr;
typedef std::vector<list_item_ptr> item_ptr_vector;

TEST(sp_intrusive_list, lifetime_of_1st_pushed)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item_ptr item = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item);
      ASSERT_EQ(1, instance_counter);
    }
    ASSERT_EQ(1, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(sp_intrusive_list, lifetime_of_1st_pushed)

TEST(sp_intrusive_list, lifetime_of_2nd_pushed)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item_ptr item1 = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item1);

      list_item_ptr item2 = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item2);

      ASSERT_EQ(2, instance_counter);
    }
    ASSERT_EQ(2, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(sp_intrusive_list, lifetime_of_2nd_pushed)

TEST(sp_intrusive_list, lifetime_of_single_erased)
{
  int instance_counter = 0;
  list_type list;
  {
    list_item_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    list.push_front(item);
    ASSERT_EQ(1, instance_counter);
    list.erase(item);
    ASSERT_EQ(1, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(sp_intrusive_list, lifetime_of_single_erased)

TEST(sp_intrusive_list, lifetime_of_1st_erased)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item_ptr item1 = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item1);

      list_item_ptr item2 = detail::make_shared<list_item>(
          detail::ref(instance_counter));

      list.push_front(item2);
      ASSERT_EQ(2, instance_counter);

      list.erase(item1);
    }
    ASSERT_EQ(1, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(sp_intrusive_list, lifetime_of_1st_erased)

TEST(sp_intrusive_list, lifetime_of_2nd_erased)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item_ptr item1 = detail::make_shared<list_item>(
          detail::ref(instance_counter));
      list.push_front(item1);
      {
        list_item_ptr item2 = detail::make_shared<list_item>(
            detail::ref(instance_counter));
        list.push_front(item2);
        ASSERT_EQ(2, instance_counter);
        list.erase(item2);
      }
      ASSERT_EQ(1, instance_counter);
    }
    ASSERT_EQ(1, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(sp_intrusive_list, lifetime_of_2nd_erased)

TEST(sp_intrusive_list, front_after_push_front)
{
  int instance_counter = 0;
  list_type list;

  list_item_ptr item1 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item1);
  ASSERT_EQ(item1, list.front());

  list_item_ptr item2 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item2);
  ASSERT_EQ(item2, list.front());
} // TEST(sp_intrusive_list, front_after_push_front)

TEST(sp_intrusive_list, front_after_erase_last)
{
  int instance_counter = 0;
  list_type list;

  list_item_ptr item1 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item1);

  list_item_ptr item2 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item2);

  list.erase(item1);
  ASSERT_EQ(item2, list.front());
} // TEST(sp_intrusive_list, front_after_erase_last)

TEST(sp_intrusive_list, front_after_erase_front)
{
  int instance_counter = 0;
  list_type list;

  list_item_ptr item1 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item1);

  list_item_ptr item2 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item2);

  list.erase(item2);
  ASSERT_EQ(item1, list.front());
} // TEST(sp_intrusive_list, front_after_erase_front)

TEST(sp_intrusive_list, size)
{
  int instance_counter = 0;
  list_type list;

  list_item_ptr item1 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item1);
  ASSERT_EQ(1U, list.size());

  list_item_ptr item2 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item2);
  ASSERT_EQ(2U, list.size());

  list.erase(item1);
  ASSERT_EQ(1U, list.size());

  list.erase(item2);
  ASSERT_EQ(0U, list.size());

  list.push_front(item1);
  list.push_front(item2);
  ASSERT_EQ(2U, list.size());

  list.clear();
  ASSERT_EQ(0U, list.size());
} // TEST(sp_intrusive_list, size)

TEST(sp_intrusive_list, empty)
{
  int instance_counter = 0;
  list_type list;
  ASSERT_TRUE(list.empty());

  list_item_ptr item1 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item1);
  ASSERT_FALSE(list.empty());

  list_item_ptr item2 = detail::make_shared<list_item>(
      detail::ref(instance_counter));
  list.push_front(item2);
  ASSERT_FALSE(list.empty());

  list.erase(item1);
  ASSERT_FALSE(list.empty());

  list.erase(item2);
  ASSERT_TRUE(list.empty());

  list.push_front(item1);
  list.push_front(item2);
  ASSERT_FALSE(list.empty());

  list.clear();
  ASSERT_TRUE(list.empty());
} // TEST(sp_intrusive_list, empty)

TEST(sp_intrusive_list, next)
{
  const std::size_t item_num = 10;
  int instance_counter = 0;
  list_type list;

  item_ptr_vector items;
  items.reserve(item_num);
  
  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list.push_front(item);
  }
  
  list_item_ptr item = list.front();
  for (item_ptr_vector::reverse_iterator i = items.rbegin(), end = items.rend();
      i != end; ++i)
  {
    ASSERT_EQ(*i, item);
    item = list_type::next(item);
  }

  ASSERT_FALSE(item);
} // TEST(sp_intrusive_list, next)

TEST(sp_intrusive_list, prev)
{
  const std::size_t item_num = 10;
  int instance_counter = 0;
  list_type list;

  item_ptr_vector items;
  items.reserve(item_num);

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list.push_front(item);
  }

  // Find last item in list
  list_item_ptr item = list.front();
  for (list_item_ptr next_item = item; next_item;
      next_item = list_type::next(item))
  {
    item = next_item;
  }

  for (item_ptr_vector::iterator i = items.begin(), end = items.end(); i != end;
      ++i)
  {
    ASSERT_EQ(*i, item);
    item = list_type::prev(item);
  }

  ASSERT_FALSE(item);
} // TEST(sp_intrusive_list, prev)

} // namespace sp_intrusive_list
} // namespace test
} // namespace ma
