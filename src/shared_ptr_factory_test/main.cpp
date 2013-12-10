//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <exception>
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <ma/config.hpp>
#include <ma/shared_ptr_factory.hpp>

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
#include <memory>
#else
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#endif // defined(MA_USE_CXX11_STDLIB_MEMORY)

namespace ma {
namespace test {

namespace shared_ptr_factory {

void run_test();

} // namespace shared_ptr_factory

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
    ma::test::shared_ptr_factory::run_test();
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

namespace shared_ptr_factory {

class A;
typedef MA_SHARED_PTR<A> A_ptr;

class A : private boost::noncopyable
{
public:
  static A_ptr create()
  {
    typedef ma::shared_ptr_factory_helper<A> A_helper;
    return MA_MAKE_SHARED<A_helper>();
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

void run_test()
{  
  {
    //BOOST_STATIC_ASSERT_MSG(!std::is_constructible<A>::value, 
    //    "class A has to be not constructible");
    MA_SHARED_PTR<A> a = A::create();
  }
  
  {    
    //BOOST_STATIC_ASSERT_MSG(!(std::is_constructible<B, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<B> B_helper;
    MA_SHARED_PTR<B> b = MA_MAKE_SHARED<B_helper>(i, j);
    BOOST_ASSERT_MSG(i == b->get_i(), "Instance has different data");
    BOOST_ASSERT_MSG(j == b->get_j(), "Instance has different data");
  }

  {
    //BOOST_STATIC_ASSERT_MSG(
    //    !(std::is_constructible<C, double, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<C> C_helper;  
    MA_SHARED_PTR<C> c = MA_MAKE_SHARED<C_helper>(d, i, j);
    BOOST_ASSERT_MSG(i == c->get_i(), "Instance has different data");
    BOOST_ASSERT_MSG(j == c->get_j(), "Instance has different data");    
    BOOST_ASSERT_MSG(d == c->get_d(), "Instance has different data");
  }
}

} // namespace shared_ptr_factory

} // namespace test
} // namespace ma
