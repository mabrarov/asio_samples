//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include <ma/detail/intrusive_list.hpp>
#include <ma/detail/utility.hpp>
#include <ma/detail/memory.hpp>

namespace ma {
namespace test {
namespace intrusive_list {

class list_item : private ma::detail::intrusive_list<list_item>::base_hook
{
private:
  typedef list_item this_type;
  typedef ma::detail::intrusive_list<list_item>::base_hook base_type;

  friend class ma::detail::intrusive_list<list_item>;

  this_type& operator=(const this_type&);

public:
  explicit list_item(int& counter) : counter_(counter)
  {
    ++counter_;
  }

  list_item(const this_type& other)
    : base_type(static_cast<const base_type&>(other))
    , counter_(other.counter_)
  {
  }

#if defined(MA_HAS_RVALUE_REFS)

  list_item(this_type&& other)
      : base_type(detail::move(other))
      , counter_(other.counter_)
  {
  }

#endif

  ~list_item()
  {
    --counter_;
  }

private:
  int& counter_;
}; // class list_item

typedef ma::detail::intrusive_list<list_item> list_type;
typedef list_item* list_item_ptr;
typedef std::vector<list_item_ptr> item_ptr_vector;

TEST(intrusive_list, empty)
{
  int instance_counter = 0;
  list_type list;
  ASSERT_TRUE(list.empty());

  list_item item1(instance_counter);
  list.push_front(item1);
  ASSERT_FALSE(list.empty());

  list_item item2(instance_counter);
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
} // TEST(intrusive_list, empty)

TEST(intrusive_list, copy_construct)
{
  int instance_counter = 0;
  list_type list1;
  list_item item1(instance_counter);
  list_item item2(instance_counter);
  list1.push_front(item1);
  list1.push_front(item2);

  list_type list2(list1);
  ASSERT_FALSE(list1.empty());
  ASSERT_FALSE(list2.empty());
  ASSERT_EQ(list1.front(), list2.front());
  ASSERT_EQ(list1.back(), list2.back());

  list1.clear();
  ASSERT_TRUE(list1.empty());
  ASSERT_FALSE(list2.empty());
} // TEST(intrusive_list, copy_construct)

TEST(intrusive_list, copy_assign)
{
  int instance_counter = 0;
  list_type list1;
  list_item item1(instance_counter);
  list_item item2(instance_counter);
  list1.push_front(item1);
  list1.push_front(item2);

  list_type list2;
  ASSERT_TRUE(list2.empty());

  list2 = list1;
  ASSERT_FALSE(list1.empty());
  ASSERT_FALSE(list2.empty());
  ASSERT_EQ(list1.front(), list2.front());
  ASSERT_EQ(list1.back(), list2.back());

  list1.clear();
  ASSERT_TRUE(list1.empty());
  ASSERT_FALSE(list2.empty());
} // TEST(intrusive_list, copy_assign)

#if defined(MA_HAS_RVALUE_REFS)

TEST(intrusive_list, move_construct)
{
  int instance_counter = 0;
  list_type list1;
  list_item item1(instance_counter);
  list_item item2(instance_counter);
  list1.push_front(item1);
  list1.push_front(item2);
  list_item_ptr front = list1.front();
  list_item_ptr back = list1.back();

  list_type list2(detail::move(list1));
  ASSERT_TRUE(list1.empty());
  ASSERT_FALSE(list2.empty());
  ASSERT_EQ(front, list2.front());
  ASSERT_EQ(back, list2.back());
} // TEST(intrusive_list, move_construct)

TEST(intrusive_list, move_assign)
{
  int instance_counter = 0;
  list_type list1;
  list_item item1(instance_counter);
  list_item item2(instance_counter);
  list1.push_front(item1);
  list1.push_front(item2);
  list_item_ptr front = list1.front();
  list_item_ptr back = list1.back();

  list_type list2;
  ASSERT_TRUE(list2.empty());
  list2 = detail::move(list1);
  ASSERT_TRUE(list1.empty());
  ASSERT_FALSE(list2.empty());
  ASSERT_EQ(front, list2.front());
  ASSERT_EQ(back, list2.back());
} // TEST(intrusive_list, move_assign)

#endif // defined(MA_HAS_RVALUE_REFS)

TEST(intrusive_list, push_one)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item item(instance_counter);
      list.push_front(item);

      ASSERT_EQ(list.front(), detail::addressof(item));
      ASSERT_EQ(list.back(), detail::addressof(item));
      ASSERT_EQ(1, instance_counter);
    }
    ASSERT_EQ(0, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, push_one)

TEST(intrusive_list, push_two)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_front(item1);

      list_item item2(instance_counter);
      list.push_front(item2);

      ASSERT_EQ(list.front(), detail::addressof(item2));
      ASSERT_EQ(list.back(), detail::addressof(item1));
      ASSERT_EQ(2, instance_counter);
    }
    ASSERT_EQ(0, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, push_two)

TEST(intrusive_list, push_three)
{
  int instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_front(item1);

      list_item item2(instance_counter);
      list.push_front(item2);

      list_item item3(instance_counter);
      list.push_front(item3);

      ASSERT_EQ(list.front(), detail::addressof(item3));
      ASSERT_EQ(list.back(), detail::addressof(item1));
      ASSERT_EQ(3, instance_counter);
    }
    ASSERT_EQ(0, instance_counter);
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, push_three)

TEST(intrusive_list, erase_last)
{
  int instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);

    ASSERT_EQ(2, instance_counter);
    list.erase(item1);
    ASSERT_EQ(2, instance_counter);

    ASSERT_EQ(list.front(), detail::addressof(item2));
    ASSERT_EQ(list.back(), detail::addressof(item2));
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, erase_last)

TEST(intrusive_list, erase_first)
{
  int instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);

    ASSERT_EQ(2, instance_counter);
    list.erase(item2);
    ASSERT_EQ(2, instance_counter);

    ASSERT_EQ(list.front(), detail::addressof(item1));
    ASSERT_EQ(list.back(), detail::addressof(item1));
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, erase_first)

TEST(intrusive_list, erase_middle)
{
  int instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);
    list_item item3(instance_counter);
    list.push_front(item3);

    ASSERT_EQ(3, instance_counter);
    list.erase(item2);
    ASSERT_EQ(3, instance_counter);

    ASSERT_EQ(list.front(), detail::addressof(item3));
    ASSERT_EQ(list.back(), detail::addressof(item1));
  }
  ASSERT_EQ(0, instance_counter);
} // TEST(intrusive_list, erase_middle)

} // namespace intrusive_list
} // namespace test
} // namespace ma
