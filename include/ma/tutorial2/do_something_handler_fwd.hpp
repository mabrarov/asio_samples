//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_TUTORIAL2_DO_SOMETHING_HANDLER_FWD_HPP
#define MA_TUTORIAL2_DO_SOMETHING_HANDLER_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/shared_ptr.hpp>

namespace ma {
namespace tutorial2 {

class do_something_handler;
typedef boost::shared_ptr<do_something_handler> do_something_handler_ptr;

} // namespace tutorial2
} // namespace ma

#endif // MA_TUTORIAL2_DO_SOMETHING_HANDLER_FWD_HPP
