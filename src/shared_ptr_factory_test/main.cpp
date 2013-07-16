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
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <ma/shared_ptr_factory.hpp>

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
typedef boost::shared_ptr<A> A_ptr;

class A : private boost::noncopyable
{
public:
  static A_ptr create()
  {
    typedef ma::shared_ptr_factory_helper<A> A_helper;
    return boost::make_shared<A_helper>();
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

void run_test()
{  
  {
    //BOOST_STATIC_ASSERT_MSG(!std::is_constructible<A>::value, 
    //    "class A has to be not constructible");
    boost::shared_ptr<A> a = A::create();
  }
  
  {    
    //BOOST_STATIC_ASSERT_MSG(!(std::is_constructible<B, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<B> B_helper;
    boost::shared_ptr<B> b = boost::make_shared<B_helper>(4, 2);
  }

  {
    //BOOST_STATIC_ASSERT_MSG(
    //    !(std::is_constructible<C, double, int, int>::value),
    //    "class B has to be not constructible");

    typedef ma::shared_ptr_factory_helper<C> C_helper;  
    boost::shared_ptr<C> c = boost::make_shared<C_helper>(1.0, 4, 2);
  }
}

} // namespace shared_ptr_factory

} // namespace test
} // namespace ma
