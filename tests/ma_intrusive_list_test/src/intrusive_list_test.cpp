//
// Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <vector>
#include <gtest/gtest.h>
#include <ma/detail/intrusive_list.hpp>
#include <ma/detail/utility.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>

namespace ma {
namespace test {
namespace intrusive_list {

class list_item : private ma::detail::intrusive_list<list_item>::base_hook
{
private:
  typedef list_item this_type;
  typedef ma::detail::intrusive_list<list_item>::base_hook base_type;

  friend class ma::detail::intrusive_list<list_item>;

public:
  explicit list_item(std::size_t& counter)
    : counter_(detail::addressof(counter))
  {
    ++(*counter_);
  }

  list_item(const this_type& other)
    : base_type(static_cast<const base_type&>(other))
    , counter_(other.counter_)
  {
  }

  this_type& operator=(const this_type& other)
  {
    this->base_type::operator=(other);
    counter_ = other.counter_;
    return *this;
  }

#if defined(MA_HAS_RVALUE_REFS)

  list_item(this_type&& other)
    : base_type(detail::move(other))
    , counter_(other.counter_)
  {
  }

  this_type& operator=(this_type&& other)
  {
    this->base_type::operator=(detail::move(other));
    counter_ = other.counter_;
    return *this;
  }

#endif

  ~list_item()
  {
    --(*counter_);
  }

private:
  std::size_t* counter_;
}; // class list_item

typedef ma::detail::intrusive_list<list_item> list_type;
typedef list_item* list_item_ptr;
typedef detail::shared_ptr<list_item> list_item_shared_ptr;
typedef std::vector<list_item_shared_ptr> item_shared_ptr_vector;

void assert_same_items(const item_shared_ptr_vector& items,
    const list_type& list)
{
  list_item_ptr item = list.front();
  for (item_shared_ptr_vector::const_reverse_iterator i = items.rbegin(),
      end = items.rend(); i != end; ++i)
  {
    ASSERT_EQ(i->get(), item);
    item = list_type::next(*item);
  }
  ASSERT_FALSE(item);
}

TEST(intrusive_list, empty)
{
  std::size_t instance_counter = 0;
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
  std::size_t instance_counter = 0;
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
  std::size_t instance_counter = 0;
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
  std::size_t instance_counter = 0;
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
  std::size_t instance_counter = 0;
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

TEST(intrusive_list, push_front_1)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item(instance_counter);
      list.push_front(item);

      ASSERT_EQ(detail::addressof(item), list.front());
      ASSERT_EQ(detail::addressof(item), list.back());
      ASSERT_EQ(1U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_front_1)

TEST(intrusive_list, push_front_2)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_front(item1);

      list_item item2(instance_counter);
      list.push_front(item2);

      ASSERT_EQ(detail::addressof(item2), list.front());
      ASSERT_EQ(detail::addressof(item1), list.back());
      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_front_2)

TEST(intrusive_list, push_front_3)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_front(item1);

      list_item item2(instance_counter);
      list.push_front(item2);

      list_item item3(instance_counter);
      list.push_front(item3);

      ASSERT_EQ(detail::addressof(item3), list.front());
      ASSERT_EQ(detail::addressof(item1), list.back());
      ASSERT_EQ(3U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_front_3)

TEST(intrusive_list, push_back_1)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item(instance_counter);
      list.push_back(item);

      ASSERT_EQ(detail::addressof(item), list.front());
      ASSERT_EQ(detail::addressof(item), list.back());
      ASSERT_EQ(1U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_back_1)

TEST(intrusive_list, push_back_2)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_back(item1);

      list_item item2(instance_counter);
      list.push_back(item2);

      ASSERT_EQ(detail::addressof(item1), list.front());
      ASSERT_EQ(detail::addressof(item2), list.back());
      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_back_2)

TEST(intrusive_list, push_back_3)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list.push_back(item1);

      list_item item2(instance_counter);
      list.push_back(item2);

      list_item item3(instance_counter);
      list.push_back(item3);

      ASSERT_EQ(detail::addressof(item1), list.front());
      ASSERT_EQ(detail::addressof(item3), list.back());
      ASSERT_EQ(3U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, push_back_3)

TEST(intrusive_list, pop_front_1)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item(instance_counter);
      list.push_front(item);
      list.pop_front();
      ASSERT_TRUE(list.empty());
      ASSERT_EQ(1U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_front_1)

TEST(intrusive_list, pop_front_2)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list.push_front(item1);
      list.push_front(item2);

      list.pop_front();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item1), list.front());
      ASSERT_EQ(detail::addressof(item1), list.back());

      list.pop_front();
      ASSERT_TRUE(list.empty());

      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_front_2)

TEST(intrusive_list, pop_front_3)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list_item item3(instance_counter);
      list.push_front(item1);
      list.push_front(item2);
      list.push_front(item3);

      list.pop_front();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item2), list.front());
      ASSERT_EQ(detail::addressof(item1), list.back());

      list.pop_front();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item1), list.front());
      ASSERT_EQ(detail::addressof(item1), list.back());

      list.pop_front();
      ASSERT_TRUE(list.empty());

      ASSERT_EQ(3U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_front_3)

TEST(intrusive_list, pop_back_1)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item(instance_counter);
      list.push_front(item);
      list.pop_back();
      ASSERT_TRUE(list.empty());
      ASSERT_EQ(1U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_back_1)

TEST(intrusive_list, pop_back_2)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list.push_front(item1);
      list.push_front(item2);

      list.pop_back();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item2), list.front());
      ASSERT_EQ(detail::addressof(item2), list.back());

      list.pop_back();
      ASSERT_TRUE(list.empty());

      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_back_2)

TEST(intrusive_list, pop_back_3)
{
  std::size_t instance_counter = 0;
  {
    list_type list;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list_item item3(instance_counter);
      list.push_front(item1);
      list.push_front(item2);
      list.push_front(item3);

      list.pop_back();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item3), list.front());
      ASSERT_EQ(detail::addressof(item2), list.back());

      list.pop_back();
      ASSERT_FALSE(list.empty());
      ASSERT_EQ(detail::addressof(item3), list.front());
      ASSERT_EQ(detail::addressof(item3), list.back());

      list.pop_back();
      ASSERT_TRUE(list.empty());

      ASSERT_EQ(3U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, pop_back_3)

TEST(intrusive_list, erase_last)
{
  std::size_t instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);

    ASSERT_EQ(2U, instance_counter);
    list.erase(item1);
    ASSERT_EQ(2U, instance_counter);

    ASSERT_EQ(detail::addressof(item2), list.front());
    ASSERT_EQ(detail::addressof(item2), list.back());
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, erase_last)

TEST(intrusive_list, erase_first)
{
  std::size_t instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);

    ASSERT_EQ(2U, instance_counter);
    list.erase(item2);
    ASSERT_EQ(2U, instance_counter);

    ASSERT_EQ(detail::addressof(item1), list.front());
    ASSERT_EQ(detail::addressof(item1), list.back());
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, erase_first)

TEST(intrusive_list, erase_middle)
{
  std::size_t instance_counter = 0;
  {
    list_type list;

    list_item item1(instance_counter);
    list.push_front(item1);
    list_item item2(instance_counter);
    list.push_front(item2);
    list_item item3(instance_counter);
    list.push_front(item3);

    ASSERT_EQ(3U, instance_counter);
    list.erase(item2);
    ASSERT_EQ(3U, instance_counter);

    ASSERT_EQ(detail::addressof(item3), list.front());
    ASSERT_EQ(detail::addressof(item1), list.back());
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, erase_middle)

TEST(intrusive_list, swap_with_empty)
{
  std::size_t instance_counter = 0;
  {
    list_type list1;
    list_type list2;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list1.push_front(item1);
      list1.push_front(item2);

      list1.swap(list2);

      ASSERT_TRUE(list1.empty());
      ASSERT_FALSE(list2.empty());

      ASSERT_EQ(detail::addressof(item2), list2.front());
      ASSERT_EQ(detail::addressof(item1), list2.back());
      ASSERT_EQ(static_cast<list_item_ptr>(0), list1.front());
      ASSERT_EQ(static_cast<list_item_ptr>(0), list1.back());

      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  {
    list_type list1;
    list_type list2;
    {
      list_item item1(instance_counter);
      list_item item2(instance_counter);
      list1.push_front(item1);
      list1.push_front(item2);

      list2.swap(list1);

      ASSERT_TRUE(list1.empty());
      ASSERT_FALSE(list2.empty());

      ASSERT_EQ(detail::addressof(item2), list2.front());
      ASSERT_EQ(detail::addressof(item1), list2.back());
      ASSERT_EQ(static_cast<list_item_ptr>(0), list1.front());
      ASSERT_EQ(static_cast<list_item_ptr>(0), list1.back());

      ASSERT_EQ(2U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, swap_with_empty)

TEST(intrusive_list, swap_with_non_empty)
{
  std::size_t instance_counter = 0;
  {
    list_type list1;
    list_type list2;
    {
      list_item item11(instance_counter);
      list_item item12(instance_counter);
      list_item item21(instance_counter);
      list_item item22(instance_counter);
      list1.push_front(item11);
      list1.push_front(item12);
      list2.push_front(item21);
      list2.push_front(item22);

      list1.swap(list2);

      ASSERT_FALSE(list1.empty());
      ASSERT_FALSE(list2.empty());

      ASSERT_EQ(detail::addressof(item12), list2.front());
      ASSERT_EQ(detail::addressof(item11), list2.back());
      ASSERT_EQ(detail::addressof(item22), list1.front());
      ASSERT_EQ(detail::addressof(item21), list1.back());

      ASSERT_EQ(4U, instance_counter);
    }
    ASSERT_EQ(0U, instance_counter);
  }
  ASSERT_EQ(0U, instance_counter);
} // TEST(intrusive_list, swap_with_non_empty)

TEST(intrusive_list, next)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num);
  list_type list;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list_item_ptr item = list.front();
  for (item_shared_ptr_vector::reverse_iterator i = items.rbegin(),
      end = items.rend(); i != end; ++i)
  {
    ASSERT_EQ(i->get(), item);
    item = list_type::next(*item);
  }

  ASSERT_FALSE(item);
} // TEST(intrusive_list, next)

TEST(intrusive_list, prev)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num);
  list_type list;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list_item_ptr item = list.back();
  for (item_shared_ptr_vector::iterator i = items.begin(), end = items.end();
      i != end; ++i)
  {
    ASSERT_EQ(i->get(), item);
    item = list_type::prev(*item);
  }

  ASSERT_FALSE(item);
} // TEST(intrusive_list, prev)

TEST(intrusive_list, insert_front_empty)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list1.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list1.insert_front(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  assert_same_items(items, list1);
} // TEST(intrusive_list, insert_front_empty)

TEST(intrusive_list, empty_insert_front)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list2.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list1.insert_front(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  assert_same_items(items, list1);
} // TEST(intrusive_list, empty_insert_front)

TEST(intrusive_list, empty_insert_front_empty)
{
  list_type list1;
  list_type list2;

  list1.insert_front(list2);

  ASSERT_TRUE(list1.empty());
  ASSERT_TRUE(list2.empty());
} // TEST(intrusive_list, empty_insert_front_empty)

TEST(intrusive_list, insert_front)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list1.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list2.push_front(*item);
  }

  ASSERT_EQ(item_num * 2, instance_counter);

  list1.insert_front(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  assert_same_items(items, list1);
} // TEST(intrusive_list, insert_front)

TEST(intrusive_list, insert_back_empty)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list1.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list1.insert_back(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  assert_same_items(items, list1);
} // TEST(intrusive_list, insert_back_empty)

TEST(intrusive_list, empty_insert_back)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list2.push_front(*item);
  }

  ASSERT_EQ(item_num, instance_counter);

  list1.insert_back(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  assert_same_items(items, list1);
} // TEST(intrusive_list, empty_insert_back)

TEST(intrusive_list, empty_insert_back_empty)
{
  list_type list1;
  list_type list2;

  list1.insert_back(list2);

  ASSERT_TRUE(list1.empty());
  ASSERT_TRUE(list2.empty());
} // TEST(intrusive_list, empty_insert_back_empty)

TEST(intrusive_list, insert_back)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items;
  items.reserve(item_num * 2);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list1.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items.push_back(item);
    list2.push_front(*item);
  }

  ASSERT_EQ(item_num * 2, instance_counter);

  list1.insert_back(list2);

  ASSERT_FALSE(list1.empty());
  ASSERT_TRUE(list2.empty());

  list_item_ptr item = list1.front();
  for (item_shared_ptr_vector::const_reverse_iterator i =
      items.rbegin() + item_num, end = items.rend(); i != end; ++i)
  {
    ASSERT_EQ(i->get(), item);
    item = list_type::next(*item);
  }
  for (item_shared_ptr_vector::const_reverse_iterator i = items.rbegin(),
      end = items.rbegin() + item_num; i != end; ++i)
  {
    ASSERT_EQ(i->get(), item);
    item = list_type::next(*item);
  }

  ASSERT_FALSE(item);
} // TEST(intrusive_list, insert_back)

TEST(intrusive_list, item_copy_assign)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items1;
  item_shared_ptr_vector items2;
  items1.reserve(item_num);
  items2.reserve(item_num);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items1.push_back(item);
    list1.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items2.push_back(item);
    list2.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    *(items1[i]) = *(items2[i]);
  }

  ASSERT_EQ(item_num * 2, instance_counter);

  assert_same_items(items1, list1);
  assert_same_items(items2, list2);
} // TEST(intrusive_list, item_copy_assign)

TEST(intrusive_list, item_move_assign)
{
  const std::size_t item_num = 10;
  std::size_t instance_counter = 0;
  item_shared_ptr_vector items1;
  item_shared_ptr_vector items2;
  items1.reserve(item_num);
  items2.reserve(item_num);
  list_type list1;
  list_type list2;

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items1.push_back(item);
    list1.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    list_item_shared_ptr item = detail::make_shared<list_item>(
        detail::ref(instance_counter));
    items2.push_back(item);
    list2.push_front(*item);
  }

  for (std::size_t i = 0; i < item_num; ++i)
  {
    *(items1[i]) = detail::move(*(items2[i]));
  }

  ASSERT_EQ(item_num * 2, instance_counter);

  assert_same_items(items1, list1);
  assert_same_items(items2, list2);
} // TEST(intrusive_list, item_move_assign)

} // namespace intrusive_list
} // namespace test
} // namespace ma
