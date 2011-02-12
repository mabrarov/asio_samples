//
// // Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
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

namespace ma_asio_handler_invoke_helpers
{
  template <typename Function, typename Context>
  inline void invoke(const Function& function, Context& context)
  {
    using namespace boost::asio;
    asio_handler_invoke(function, context);
  }  // invoke

} // namespace ma_asio_handler_invoke_helpers

#endif // MA_HANDLER_INVOKE_HELPERS_HPP