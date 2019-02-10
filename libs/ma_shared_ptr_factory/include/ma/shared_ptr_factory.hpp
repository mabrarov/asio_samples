//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_SHARED_PTR_FACTORY_HPP
#define MA_SHARED_PTR_FACTORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ma/config.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

template <typename T>
struct shared_ptr_factory_helper : T
{
  shared_ptr_factory_helper()
    : T()
  {
  }

  template <typename Arg1>
  explicit shared_ptr_factory_helper(MA_FWD_REF(Arg1) arg1)
    : T(detail::forward<Arg1>(arg1))
  {
  }

  template <typename Arg1, typename Arg2>
  shared_ptr_factory_helper(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2)
    : T(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  shared_ptr_factory_helper(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3)
    : T(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
          detail::forward<Arg3>(arg3))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  shared_ptr_factory_helper(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4)
    : T(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
          detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4))
  {
  }

  template <typename Arg1, typename Arg2, typename Arg3,
      typename Arg4, typename Arg5>
  shared_ptr_factory_helper(MA_FWD_REF(Arg1) arg1, MA_FWD_REF(Arg2) arg2,
      MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4, MA_FWD_REF(Arg5) arg5)
    : T(detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
          detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4),
          detail::forward<Arg5>(arg5))
  {
  }

}; // struct shared_ptr_factory_helper

} // namespace ma

#endif // MA_SHARED_PTR_FACTORY_HPP
