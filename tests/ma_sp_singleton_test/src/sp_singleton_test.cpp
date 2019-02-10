//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstddef>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <gtest/gtest.h>
#include <ma/config.hpp>
#include <ma/thread_group.hpp>
#include <ma/limited_int.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/sp_singleton.hpp>
#include <ma/detail/random.hpp>

namespace ma {
namespace test {

template <typename Integer>
std::string integer_to_string(Integer value)
{
  return boost::lexical_cast<std::string>(static_cast<boost::uintmax_t>(value));
}

template <typename Integer>
std::string to_string(const ma::limited_int<Integer>& limited_value)
{
  if (limited_value.overflowed())
  {
    return ">" + integer_to_string(limited_value.value());
  }
  else
  {
    return integer_to_string(limited_value.value());
  }
}

namespace sp_singleton_construction {

class foo;
typedef detail::shared_ptr<foo> foo_ptr;

class foo : private boost::noncopyable
{
private:
  typedef foo this_type;

public:
  static foo_ptr get_nullable_instance();
  static foo_ptr get_instance();

  int data() const;

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  foo(const instance_guard_type&, int);
  ~foo();

private:
  struct factory;

  instance_guard_type instance_guard_;
  int data_;
}; // class foo

TEST(sp_singleton, construction)
{
  std::vector<int> data;
  for (std::size_t i = 0; i != 2; ++i)
  {
    foo_ptr foo0 = foo::get_nullable_instance();
    ASSERT_FALSE(static_cast<bool>(foo0));
    const foo_ptr foo1 = foo::get_instance();
    const foo_ptr foo2 = foo::get_instance();
    ASSERT_EQ(foo1->data(), foo2->data());
    foo0 = foo::get_nullable_instance();
    ASSERT_TRUE(static_cast<bool>(foo0));
    ASSERT_EQ(foo0->data(), foo2->data());
    data.push_back(foo0->data());
  }
  ASSERT_NE(data[0], data[1]);
} // TEST(sp_singleton, construction)

struct foo::factory
{
  foo_ptr operator()(const instance_guard_type& instance_guard)
  {
    typedef ma::shared_ptr_factory_helper<foo> helper;
    static int data = 0;
    return detail::make_shared<helper>(instance_guard, data++);
  }
}; // struct factory

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  return detail::sp_singleton<foo>::get_instance(factory());
}

int foo::data() const
{
  return data_;
}

foo::foo(const instance_guard_type& instance_guard, int data)
  : instance_guard_(instance_guard)
  , data_(data)
{
}

foo::~foo()
{
}

} // namespace sp_singleton_construction

namespace sp_singleton_thread {

class foo;
typedef detail::shared_ptr<foo> foo_ptr;
typedef detail::weak_ptr<foo>   foo_weak_ptr;

class foo : private boost::noncopyable
{
private:
  typedef foo this_type;

public:
  static foo_ptr get_nullable_instance();
  static foo_ptr get_instance();

  int data() const;

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  foo(const instance_guard_type&, int);
  ~foo();

private:
  struct factory;

  instance_guard_type instance_guard_;
  int data_;
}; // class foo

void thread_func(foo_ptr foo)
{
  (void) foo->data();
}

void thread_func2(const foo_weak_ptr& weak_foo, detail::latch& latch1,
    detail::latch& latch2, detail::latch& latch3)
{
  if (foo_ptr foo = weak_foo.lock())
  {
    latch1.count_down();
    (void) foo->data();
    latch2.wait();
  }
  latch3.count_down();
}

TEST(sp_singleton, thread)
{
  {
    detail::thread t(detail::bind(thread_func, foo::get_instance()));
    t.join();
    const foo_ptr foo = foo::get_instance();
    ASSERT_EQ(1, foo->data());
  }

  {
    detail::latch latch1(1);
    detail::latch latch2(1);
    detail::latch latch3(1);

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
    detail::unique_ptr<detail::thread> t;
#else
    detail::scoped_ptr<detail::thread> t;
#endif

    {
      const foo_ptr thread_foo = foo::get_instance();
      t.reset(new detail::thread(detail::bind(thread_func2,
          foo_weak_ptr(thread_foo), detail::ref(latch1), detail::ref(latch2),
          detail::ref(latch3))));
      latch1.wait();
    }
    {
      const foo_ptr foo = foo::get_nullable_instance();
      latch2.count_down();
      ASSERT_TRUE(static_cast<bool>(foo));
    }
    {
      ma::limited_int<std::size_t> count = 0;
      while (const foo_ptr foo = foo::get_nullable_instance())
      {
        ++count;
      }
      std::cout << to_string(count) << std::endl;
    }
    {
      latch3.wait();
      const foo_ptr foo = foo::get_nullable_instance();
      ASSERT_FALSE(static_cast<bool>(foo));
    }
    t->join();
  }
} // TEST(sp_singleton, thread)

struct foo::factory
{
  foo_ptr operator()(const instance_guard_type& instance_guard)
  {
    typedef ma::shared_ptr_factory_helper<foo> helper;
    static int data = 0;
    return detail::make_shared<helper>(instance_guard, data++);
  }
}; // struct foo::factory

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  return detail::sp_singleton<foo>::get_instance(factory());
}

int foo::data() const
{
  return data_;
}

foo::foo(const instance_guard_type& instance_guard, int data)
  : instance_guard_(instance_guard)
  , data_(data)
{
}

foo::~foo()
{
}

} // namespace sp_singleton_thread

namespace sp_singleton_sync {

detail::latch destroy_start_latch;
detail::latch destroy_complete_latch;

class foo;
typedef detail::shared_ptr<foo> foo_ptr;

class foo : private boost::noncopyable
{
private:
  typedef foo this_type;

public:
  static foo_ptr get_nullable_instance();
  static foo_ptr get_instance();

  int data() const;

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  foo(const instance_guard_type&, int);
  ~foo();

private:
  struct factory;

  static std::size_t instance_count_;

  boost::optional<instance_guard_type> instance_guard_;
  int data_;
}; // class foo

std::size_t foo::instance_count_ = 0;

void thread_func()
{
  const foo_ptr f = foo::get_instance();
  ASSERT_TRUE(static_cast<bool>(f));
  ASSERT_EQ(0, f->data());
  (void) f;
}

TEST(sp_singleton, sync)
{
  {
    destroy_start_latch.count_up();
    destroy_complete_latch.count_up();
    detail::thread t(thread_func);
    destroy_start_latch.wait();
    {
      const foo_ptr f = foo::get_nullable_instance();
      ASSERT_FALSE(static_cast<bool>(f));
    }
    destroy_complete_latch.count_down();
    {
      const foo_ptr f = foo::get_instance();
      ASSERT_TRUE(static_cast<bool>(f));
      ASSERT_EQ(1, f->data());
      destroy_start_latch.count_up();
    }
    t.join();
  }
} // TEST(sp_singleton, sync)

struct foo::factory
{
  foo_ptr operator()(const instance_guard_type& instance_guard)
  {
    typedef ma::shared_ptr_factory_helper<foo> helper;
    static int data = 0;
    return detail::make_shared<helper>(instance_guard, data++);
  }
}; // struct foo::factory

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  return detail::sp_singleton<foo>::get_instance(factory());
}

int foo::data() const
{
  return data_;
}

foo::foo(const instance_guard_type& instance_guard, int data)
  : instance_guard_(instance_guard)
  , data_(data)
{
  EXPECT_EQ(0U, instance_count_);
  ++instance_count_;
}

foo::~foo()
{
  destroy_start_latch.count_down();
  destroy_complete_latch.wait();
  if (!data_)
  {
    detail::this_thread::sleep(boost::posix_time::seconds(1));
  }
  --instance_count_;
  instance_guard_ = boost::none;
}

} // namespace sp_singleton_sync

namespace sp_singleton_sync2 {

class foo;
typedef detail::shared_ptr<foo> foo_ptr;

class foo : private boost::noncopyable
{
private:
  typedef foo this_type;

public:
  typedef ma::limited_int<std::size_t> counter;

  static foo_ptr get_nullable_instance();
  static foo_ptr get_instance();

  counter init_count() const;

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  foo(const instance_guard_type&, const counter&);
  ~foo();

private:
  struct factory;

  static std::size_t instance_count_;

  boost::optional<instance_guard_type> instance_guard_;
  counter init_counter_;
}; // class foo

std::size_t foo::instance_count_   = 0;
const std::size_t iteration_count  = 1;
const std::size_t work_cycle_count = 1;

typedef detail::mt19937 random_generator;
typedef detail::shared_ptr<random_generator> random_generator_ptr;

void work_func(const random_generator_ptr& rng)
{
  detail::uniform_int_distribution<> wait_flag(0, 1);
  for (std::size_t i = 0; i != work_cycle_count; ++i)
  {
    const foo_ptr f = foo::get_instance();
    ASSERT_TRUE(static_cast<bool>(f));
    if (wait_flag(*rng))
    {
      detail::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
    (void) f;
  }
}

void thread_func(detail::barrier& work_barrier, const random_generator_ptr& rng)
{
  work_barrier.count_down_and_wait();
  work_func(rng);
}

TEST(sp_singleton, sync2)
{
  const std::size_t thread_count = 2;

  std::vector<random_generator_ptr> rngs;
  for (std::size_t i = 0; i != thread_count; ++i)
  {
    rngs.push_back(detail::make_shared<random_generator>());
  }

  for (std::size_t n = 0; n != iteration_count; ++n)
  {
    detail::barrier work_barrier(
        static_cast<detail::barrier::counter_type>(thread_count));
    ma::thread_group threads;
    for (std::size_t i = 0; i != thread_count - 1; ++i)
    {
      threads.create_thread(
          detail::bind(thread_func, detail::ref(work_barrier), rngs[i]));
    }
    thread_func(work_barrier, rngs.back());
    threads.join_all();
  }

  const foo_ptr f = foo::get_instance();
  std::cout << "Init counter: " << to_string(f->init_count()) << std::endl;
} // TEST(sp_singleton, sync2)

struct foo::factory
{
  foo_ptr operator()(const instance_guard_type& instance_guard)
  {
    typedef ma::shared_ptr_factory_helper<foo> helper;
    static counter init_counter = 0;
    ++init_counter;
    return detail::make_shared<helper>(instance_guard, init_counter);
  }
}; // struct foo::factory

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  return detail::sp_singleton<foo>::get_instance(factory());
}

foo::counter foo::init_count() const
{
  return init_counter_;
}

foo::foo(const instance_guard_type& instance_guard, const counter& init_counter)
  : instance_guard_(instance_guard)
  , init_counter_(init_counter)
{
  EXPECT_EQ(0U, instance_count_);
  ++instance_count_;
}

foo::~foo()
{
  EXPECT_EQ(1U, instance_count_);
  --instance_count_;
  instance_guard_ = boost::none;
}

} // namespace sp_singleton_sync2

} // namespace test
} // namespace ma
