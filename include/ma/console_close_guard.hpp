//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_CONSOLE_CLOSE_GUARD_HPP
#define MA_CONSOLE_CLOSE_GUARD_HPP

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
class console_close_guard : private boost::noncopyable
{
public:
  typedef boost::function<void (void)> ctrl_function_type;

  console_close_guard(const ctrl_function_type& ctrl_function);
  ~console_close_guard();  

private:
  class implementation;

  boost::scoped_ptr<implementation> implementation_;
}; // class console_close_guard

} // namespace ma

#endif // MA_CONSOLE_CLOSE_GUARD_HPP
