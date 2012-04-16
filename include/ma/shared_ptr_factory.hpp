//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_SHARED_PTR_FACTORY_HPP
#define MA_SHARED_PTR_FACTORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

template <typename T>
struct shared_ptr_factory_helper : T
{
  shared_ptr_factory_helper()
    : T()
  {
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Arg1>
  explicit shared_ptr_factory_helper(Arg1&& arg1)
    : T(std::forward<Arg1>(arg1))
  {
  }

  template <typename Arg1, typename Arg2>
  shared_ptr_factory_helper(Arg1&& arg1, Arg2&& arg2)
    : T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  shared_ptr_factory_helper(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
    : T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
          std::forward<Arg3>(arg3))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  shared_ptr_factory_helper(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
    : T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
          std::forward<Arg3>(arg3), std::forward<Arg4>(arg4))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3,
      typename Arg4, typename Arg5>
  shared_ptr_factory_helper(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3,
      Arg4&& arg4, Arg5&& arg5)
    : T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
          std::forward<Arg3>(arg3), std::forward<Arg4>(arg4),
          std::forward<Arg5>(arg5))
  {
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Arg1>
  explicit shared_ptr_factory_helper(const Arg1& arg1)
    : T(arg1)
  {
  }

  template <typename Arg1, typename Arg2>
  shared_ptr_factory_helper(const Arg1& arg1, const Arg2& arg2)
    : T(arg1, arg2)
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  shared_ptr_factory_helper(const Arg1& arg1,
      const Arg2& arg2, const Arg3& arg3)
    : T(arg1, arg2, arg3)
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  shared_ptr_factory_helper(const Arg1& arg1, const Arg2& arg2,
      const Arg3& arg3, const Arg4& arg4)
    : T(arg1, arg2, arg3, arg4)
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3,
      typename Arg4, typename Arg5>
  shared_ptr_factory_helper(const Arg1& arg1, const Arg2& arg2,
      const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
    : T(arg1, arg2, arg3, arg4, arg5)
  {
  }

#endif // defined(MA_HAS_RVALUE_REFS)

}; // struct shared_ptr_factory_helper

#endif // MA_SHARED_PTR_FACTORY_HPP
