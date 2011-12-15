//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_HANDLER_INVOKE_HELPERS_HPP
#define MA_HANDLER_INVOKE_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/utility/addressof.hpp>
#include <ma/config.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#endif // defined(MA_HAS_RVALUE_REFS)

// Calls to asio_handler_invoke must be made from a namespace that does not
// contain any overloads of this function. The ma_asio_handler_invoke_helpers
// namespace is defined here for that purpose. It's a modified copy of Asio
// sources: asio/detail/handler_invoke_helpers.hpp
namespace ma_asio_handler_invoke_helpers {

#if defined(MA_HAS_RVALUE_REFS)

template <typename Function, typename Context>
inline void invoke(Function&& function, Context& context)
{
  using namespace boost::asio;
  asio_handler_invoke(std::forward<Function>(function),
      boost::addressof(context));
}

#else // defined(MA_HAS_RVALUE_REFS)

template <typename Function, typename Context>
inline void invoke(const Function& function, Context& context)
{
  using namespace boost::asio;
  asio_handler_invoke(function, boost::addressof(context));
}

#endif // defined(MA_HAS_RVALUE_REFS)

} // namespace ma_asio_handler_invoke_helpers

#endif // MA_HANDLER_INVOKE_HELPERS_HPP
