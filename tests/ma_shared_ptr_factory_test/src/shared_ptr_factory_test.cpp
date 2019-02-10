//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>
#include <ma/shared_ptr_factory.hpp>
#include <ma/detail/memory.hpp>

namespace ma {
namespace test {
namespace shared_ptr_factory {

class A;
typedef detail::shared_ptr<A> A_ptr;

class A : private boost::noncopyable
{
public:
  static A_ptr create()
  {
    typedef ma::shared_ptr_factory_helper<A> A_helper;
    return detail::make_shared<A_helper>();
  }

protected:
  A()
  {
  }

  ~A()
  {
  }
}; // class A 

class B : private boost::noncopyable
{
public:
  int get_i() const
  {
    return i_;
  }

  int get_j() const
  {
    return j_;
  }

private:
  typedef B this_type;

  B(int i, int j)
    : i_(i)
    , j_(j)
  {
  }

  ~B()
  {
  }

  friend struct ma::shared_ptr_factory_helper<this_type>;

private:
  int i_;
  int j_;
}; // class B

class C : public A
{
public:
  double get_d() const
  {
    return d_;
  }

  int get_i() const
  {
    return i_;
  }

  int get_j() const
  {
    return j_;
  }

protected:
  C(double d, int i, int j)
    : d_(d)
    , i_(i)
    , j_(j)
  {
  }

  ~C()
  {
  }

private:
  double d_;
  int i_;
  int j_;
}; // class C

static const int    i = 4;
static const int    j = 2;
static const double d = 1.0;

TEST(shared_ptr_factory, simple)
{  
  {
    //BOOST_STATIC_ASSERT_MSG(!std::is_constructible<A>::value, 
    //    "class A has to be not constructible");
    detail::shared_ptr<A> a = A::create();
  }
  
  {    
    //BOOST_STATIC_ASSERT_MSG(!(std::is_constructible<B, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<B> B_helper;
    detail::shared_ptr<B> b = detail::make_shared<B_helper>(i, j);
    ASSERT_EQ(i, b->get_i());
    ASSERT_EQ(j, b->get_j());
  }

  {
    //BOOST_STATIC_ASSERT_MSG(
    //    !(std::is_constructible<C, double, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<C> C_helper;  
    detail::shared_ptr<C> c = detail::make_shared<C_helper>(d, i, j);
    ASSERT_EQ(i, c->get_i());
    ASSERT_EQ(j, c->get_j());
    ASSERT_EQ(d, c->get_d());
  }
} // TEST(shared_ptr_factory, simple)

} // namespace shared_ptr_factory
} // namespace test
} // namespace ma
