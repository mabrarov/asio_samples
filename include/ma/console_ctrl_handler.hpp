//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CONSOLE_CTRL_HANDLER_HPP
#define CONSOLE_CTRL_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace ma {

/// Hook-helper for console application. Supports setup of the own functor that
/// will be called when user tries to close console application.
/**
 * Supported OS: MS Windows family, Linux (Ubuntu 10.x tested).
 */
class console_ctrl_handler : private boost::noncopyable
{
public:
  typedef boost::function<void (void)> ctrl_function_type;

  console_ctrl_handler(const ctrl_function_type& ctrl_function);
  ~console_ctrl_handler();  

private:
  class implementation;

  boost::scoped_ptr<implementation> implementation_;
}; // class console_ctrl_handler

} // namespace ma

#endif // CONSOLE_CTRL_HANDLER_HPP
