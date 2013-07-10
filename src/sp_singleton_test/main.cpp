//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/random.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/limited_int.hpp>
#include <ma/shared_ptr_factory.hpp>
#include <ma/detail/sp_singleton.hpp>

namespace ma {
namespace test {

namespace sp_singleton_construction {

void run_test();

} // namespace sp_singleton_construction

namespace sp_singleton_thread {

void run_test();

} // namespace sp_singleton_thread

namespace sp_singleton_sync {

void run_test();

} // namespace sp_singleton_sync

namespace sp_singleton_sync2 {

void run_test();

} // namespace sp_singleton_sync2

} // namespace test
} // namespace ma

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{
  try
  {
    ma::test::sp_singleton_construction::run_test();
    ma::test::sp_singleton_thread::run_test();
    ma::test::sp_singleton_sync::run_test();
    ma::test::sp_singleton_sync2::run_test();
    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}

namespace ma {
namespace test {

namespace sp_singleton_construction {

class foo;
typedef boost::shared_ptr<foo> foo_ptr;

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
  instance_guard_type instance_guard_;
  int data_;
}; // class foo

void run_test()
{
  std::vector<int> data;  
  for (std::size_t i = 0; i != 2; ++i)
  {
    foo_ptr foo0 = foo::get_nullable_instance();
    BOOST_ASSERT_MSG(!foo0, "Instance has to not exist");
    const foo_ptr foo1 = foo::get_instance();
    const foo_ptr foo2 = foo::get_instance();
    BOOST_ASSERT_MSG(foo1->data() == foo2->data(), "Instances are different");
    foo0 = foo::get_nullable_instance();
    BOOST_ASSERT_MSG(foo0, "Instance has to exist");  
    BOOST_ASSERT_MSG(foo0->data() == foo2->data(), "Instances are different");
    data.push_back(foo0->data());
  }
  BOOST_ASSERT_MSG(data[0] != data[1], "Instances has to be different");
}

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  class factory
  {  
  public:
    foo_ptr operator()(const instance_guard_type& instance_guard)
    {
      typedef ma::shared_ptr_factory_helper<foo> helper;
      static int data = 0;
      return boost::make_shared<helper>(instance_guard, data++);
    }
  };
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
typedef boost::shared_ptr<foo> foo_ptr;
typedef boost::weak_ptr<foo>   foo_weak_ptr;

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
  instance_guard_type instance_guard_;
  int data_;
}; // class foo

void thread_func(foo_ptr foo)
{
  (void) foo->data();
}

void thread_func2(foo_weak_ptr weak_foo, 
  ma::detail::threshold& threshold1,
  ma::detail::threshold& threshold2,
  ma::detail::threshold& threshold3)
{
  if (foo_ptr foo = weak_foo.lock())
  {
    threshold1.dec();
    (void) foo->data();
    threshold2.wait();
  }
  threshold3.dec();
}

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

void run_test()
{
  {
    boost::thread t(boost::bind(thread_func, foo::get_instance()));
    t.join();
    const foo_ptr foo = foo::get_instance();
    BOOST_ASSERT_MSG(1 == foo->data(), "Instance has to be different");
  }

  {
    ma::detail::threshold threshold1(1);
    ma::detail::threshold threshold2(1);
    ma::detail::threshold threshold3(1);
    boost::scoped_ptr<boost::thread> t;
    {
      const foo_ptr thread_foo = foo::get_instance();
      t.reset(new boost::thread(boost::bind(
          thread_func2, foo_weak_ptr(thread_foo), 
          boost::ref(threshold1), 
          boost::ref(threshold2), 
          boost::ref(threshold3))));
      threshold1.wait();
    }
    {
      const foo_ptr foo = foo::get_nullable_instance();
      threshold2.dec();
      BOOST_ASSERT_MSG(foo, "Instance has to exist");
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
      threshold3.wait();
      const foo_ptr foo = foo::get_nullable_instance();
      BOOST_ASSERT_MSG(!foo, "Instance has to not exist");
    }    
    t->join();
  }
}

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  class factory
  {
  public:
    foo_ptr operator()(const instance_guard_type& instance_guard)
    {
      typedef ma::shared_ptr_factory_helper<foo> helper;
      static int data = 0;
      return boost::make_shared<helper>(instance_guard, data++);
    }
  };
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

ma::detail::threshold destroy_start_threshold;
ma::detail::threshold destroy_complete_threshold;

class foo;
typedef boost::shared_ptr<foo> foo_ptr;

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
  static std::size_t instance_count_;

  boost::optional<instance_guard_type> instance_guard_;
  int data_;  
}; // class foo

std::size_t foo::instance_count_ = 0;

void thread_func()
{
  const foo_ptr f = foo::get_instance();
  BOOST_ASSERT_MSG(f, "Instance has to exist");
  BOOST_ASSERT_MSG(!f->data(), "Instance has to be the first");
  (void) f;
}

void thread_func2()
{
  const foo_ptr f = foo::get_instance();
  BOOST_ASSERT_MSG(f, "Instance has to exist");
  BOOST_ASSERT_MSG(2 == f->data(), "Instance has to be the third");
  (void) f;
}

void run_test()
{
  {
    destroy_start_threshold.inc();
    destroy_complete_threshold.inc();
    boost::thread t(thread_func);
    destroy_start_threshold.wait();
    {
      const foo_ptr f = foo::get_nullable_instance();
      BOOST_ASSERT_MSG(!f, "Instance has to not exist");
    }
    destroy_complete_threshold.dec();
    {
      const foo_ptr f = foo::get_instance();      
      BOOST_ASSERT_MSG(f, "Instance has to exist");
      BOOST_ASSERT_MSG(1 == f->data(), "Instance has to be the second");
      destroy_start_threshold.inc();
    }
    t.join();
  }  
}

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  class factory
  {
  public:
    foo_ptr operator()(const instance_guard_type& instance_guard)
    {
      typedef ma::shared_ptr_factory_helper<foo> helper;
      static int data = 0;
      return boost::make_shared<helper>(instance_guard, data++);
    }
  }; // class factory
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
  BOOST_ASSERT_MSG(!instance_count_, "Instance has to be the only");
  ++instance_count_;
}

foo::~foo()
{
  destroy_start_threshold.dec();  
  destroy_complete_threshold.wait();
  if (!data_)
  {
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
  --instance_count_;
  instance_guard_ = boost::none;
}

} // namespace sp_singleton_sync

namespace sp_singleton_sync2 {

class foo;
typedef boost::shared_ptr<foo> foo_ptr;

class foo : private boost::noncopyable
{
private:
  typedef foo this_type;

public:
  typedef ma::limited_int<std::size_t> data_type;

  static foo_ptr get_nullable_instance();
  static foo_ptr get_instance();

  data_type data() const;

protected:
  typedef detail::sp_singleton<this_type>::instance_guard instance_guard_type;

  foo(const instance_guard_type&, const data_type&);
  ~foo();

private:
  static volatile std::size_t instance_count_;

  boost::optional<instance_guard_type> instance_guard_;
  data_type data_;  
}; // class foo

volatile std::size_t foo::instance_count_ = 0;

const std::size_t iteration_count         = 1000;
const std::size_t work_cycle_count        = 10000;
const std::size_t additional_thread_count = 7;

void work_func()
{
  boost::random::mt19937 rng;
  boost::random::uniform_int_distribution<> wait_flag(0, 1);
  for (std::size_t i = 0; i != work_cycle_count; ++i)
  {
    const foo_ptr f = foo::get_instance();
    BOOST_ASSERT_MSG(f, "Instance has to exist");
    if (wait_flag(rng))
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
    (void) f;
  }
}

void thread_func(boost::barrier& work_barrier)
{
  work_barrier.wait();
  work_func();
}

void run_test()
{
  for (std::size_t n = 0; n != iteration_count; ++n)
  {
    boost::barrier work_barrier(additional_thread_count);
    boost::thread_group threads;
    for (std::size_t i = 0; i != additional_thread_count; ++i)
    {
      threads.create_thread(boost::bind(thread_func, boost::ref(work_barrier)));
    }
    work_func();
    threads.join_all();
  }
}

foo_ptr foo::get_nullable_instance()
{
  return detail::sp_singleton<foo>::get_nullable_instance();
}

foo_ptr foo::get_instance()
{
  class factory
  {
  public:
    foo_ptr operator()(const instance_guard_type& instance_guard)
    {
      typedef ma::shared_ptr_factory_helper<foo> helper;
      static data_type data = 0;
      data_type local_data = data;
      ++data;
      return boost::make_shared<helper>(instance_guard, local_data);
    }
  }; // class factory
  return detail::sp_singleton<foo>::get_instance(factory());
}

foo::data_type foo::data() const
{
  return data_;
}

foo::foo(const instance_guard_type& instance_guard, const data_type& data)
  : instance_guard_(instance_guard)
  , data_(data)
{
  BOOST_ASSERT_MSG(!instance_count_, "Instance has to be the only");
  ++instance_count_;
}

foo::~foo()
{
  BOOST_ASSERT_MSG(1 == instance_count_, "Instance has to be the only");
  --instance_count_;
  instance_guard_ = boost::none;
}

} // namespace sp_singleton_sync2

} // namespace test
} // namespace ma
