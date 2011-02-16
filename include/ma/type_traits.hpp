//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TYPE_TRAITS_HPP
#define MA_TYPE_TRAITS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>
#if defined(BOOST_HAS_RVALUE_REFS)

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>

namespace ma
{  
  template <typename T>
  struct remove_cv_reference
  {
    typedef typename boost::remove_cv<
      typename boost::remove_reference<T>::type
      >::type type;
  }; // struct identity
} // namespace ma
#endif // defined(BOOST_HAS_RVALUE_REFS)

#endif // MA_TYPE_TRAITS_HPP