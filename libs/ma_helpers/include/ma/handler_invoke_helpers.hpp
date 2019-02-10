//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_INVOKE_HELPERS_HPP
#define MA_HANDLER_INVOKE_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <ma/config.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/utility.hpp>

// Calls to asio_handler_invoke must be made from a namespace that does not
// contain any overloads of this function. The ma_handler_invoke_helpers
// namespace is defined here for that purpose. It's a modified copy of Asio
// sources: asio/detail/handler_invoke_helpers.hpp
namespace ma_handler_invoke_helpers {

#if defined(MA_HAS_RVALUE_REFS)

template <typename Function, typename Context>
inline void invoke(MA_FWD_REF(Function) function, Context& context)
{
  using namespace boost::asio;
  asio_handler_invoke(ma::detail::forward<Function>(function),
      ma::detail::addressof(context));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Function, typename Context>
inline void invoke(Function& function, Context& context)
{
  using namespace boost::asio;
  asio_handler_invoke(function, ma::detail::addressof(context));
}

template <typename Function, typename Context>
inline void invoke(const Function& function, Context& context)
{
  using namespace boost::asio;
  asio_handler_invoke(function, ma::detail::addressof(context));
}

#endif // defined(MA_HAS_RVALUE_REFS)

} // namespace ma_handler_invoke_helpers

#endif // MA_HANDLER_INVOKE_HELPERS_HPP
