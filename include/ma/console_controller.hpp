//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CONSOLE_CONTROLLER_HPP
#define CONSOLE_CONTROLLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if defined(WIN32)
#include <windows.h>
#endif

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/thread/recursive_mutex.hpp>

namespace ma {

class console_controller : private boost::noncopyable
{
public:
  typedef boost::function<void (void)> ctrl_function_type;

  console_controller(const ctrl_function_type& crl_function);
  ~console_controller();

private:
  typedef boost::recursive_mutex mutex_type;

#if defined(WIN32)
  static BOOL WINAPI console_ctrl_proc(DWORD ctrl_type);
#else
  static void console_ctrl_proc(int signal); 
#endif // defined(WIN32)

  static mutex_type ctrl_mutex_;
  static ctrl_function_type ctrl_function_;	
}; // class console_controller

} // namespace ma

#endif // CONSOLE_CONTROLLER_HPP
