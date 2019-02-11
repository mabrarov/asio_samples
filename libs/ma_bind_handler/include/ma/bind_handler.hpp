//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_BIND_HANDLER_HPP
#define MA_BIND_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <ma/config.hpp>
#include <ma/detail/type_traits.hpp>
#include <ma/detail/binder.hpp>
#include <ma/detail/utility.hpp>

namespace ma {

/// Helper for creation of bound handler.
template <typename Handler, typename Arg1>
inline detail::binder1<typename detail::decay<Handler>::type,
    typename detail::decay<Arg1>::type>
bind_handler(MA_FWD_REF(Handler) handler, MA_FWD_REF(Arg1) arg1)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef typename detail::decay<Arg1>::type arg1_type;
  return detail::binder1<handler_type, arg1_type>(
      detail::forward<Handler>(handler), detail::forward<Arg1>(arg1));
}

template <typename Handler, typename Arg1, typename Arg2>
inline detail::binder2<typename detail::decay<Handler>::type,
    typename detail::decay<Arg1>::type,
    typename detail::decay<Arg2>::type>
bind_handler(MA_FWD_REF(Handler) handler, MA_FWD_REF(Arg1) arg1,
    MA_FWD_REF(Arg2) arg2)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef typename detail::decay<Arg1>::type arg1_type;
  typedef typename detail::decay<Arg2>::type arg2_type;
  return detail::binder2<handler_type, arg1_type, arg2_type>(
      detail::forward<Handler>(handler), detail::forward<Arg1>(arg1),
      detail::forward<Arg2>(arg2));
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3>
inline detail::binder3<typename detail::decay<Handler>::type,
    typename detail::decay<Arg1>::type,
    typename detail::decay<Arg2>::type,
    typename detail::decay<Arg3>::type>
bind_handler(MA_FWD_REF(Handler) handler, MA_FWD_REF(Arg1) arg1,
    MA_FWD_REF(Arg2) arg2, MA_FWD_REF(Arg3) arg3)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef typename detail::decay<Arg1>::type arg1_type;
  typedef typename detail::decay<Arg2>::type arg2_type;
  typedef typename detail::decay<Arg3>::type arg3_type;
  return detail::binder3<handler_type, arg1_type, arg2_type, arg3_type>(
      detail::forward<Handler>(handler), detail::forward<Arg1>(arg1),
      detail::forward<Arg2>(arg2), detail::forward<Arg3>(arg3));
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4>
inline detail::binder4<typename detail::decay<Handler>::type,
    typename detail::decay<Arg1>::type,
    typename detail::decay<Arg2>::type,
    typename detail::decay<Arg3>::type,
    typename detail::decay<Arg4>::type>
bind_handler(MA_FWD_REF(Handler) handler, MA_FWD_REF(Arg1) arg1,
    MA_FWD_REF(Arg2) arg2, MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef typename detail::decay<Arg1>::type arg1_type;
  typedef typename detail::decay<Arg2>::type arg2_type;
  typedef typename detail::decay<Arg3>::type arg3_type;
  typedef typename detail::decay<Arg4>::type arg4_type;
  return detail::binder4<handler_type, arg1_type, arg2_type,
      arg3_type, arg4_type>(detail::forward<Handler>(handler),
          detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
          detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4));
}

template <typename Handler, typename Arg1, typename Arg2, typename Arg3,
    typename Arg4, typename Arg5>
inline detail::binder5<typename detail::decay<Handler>::type,
    typename detail::decay<Arg1>::type,
    typename detail::decay<Arg2>::type,
    typename detail::decay<Arg3>::type,
    typename detail::decay<Arg4>::type,
    typename detail::decay<Arg5>::type>
bind_handler(MA_FWD_REF(Handler) handler, MA_FWD_REF(Arg1) arg1,
    MA_FWD_REF(Arg2) arg2, MA_FWD_REF(Arg3) arg3, MA_FWD_REF(Arg4) arg4,
    MA_FWD_REF(Arg5) arg5)
{
  typedef typename detail::decay<Handler>::type handler_type;
  typedef typename detail::decay<Arg1>::type arg1_type;
  typedef typename detail::decay<Arg2>::type arg2_type;
  typedef typename detail::decay<Arg3>::type arg3_type;
  typedef typename detail::decay<Arg4>::type arg4_type;
  typedef typename detail::decay<Arg5>::type arg5_type;
  return detail::binder5<handler_type, arg1_type, arg2_type,
      arg3_type, arg4_type, arg5_type>(detail::forward<Handler>(handler),
          detail::forward<Arg1>(arg1), detail::forward<Arg2>(arg2),
          detail::forward<Arg3>(arg3), detail::forward<Arg4>(arg4),
          detail::forward<Arg5>(arg5));
}

} // namespace ma

#endif // MA_BIND_HANDLER_HPP
