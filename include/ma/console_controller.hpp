//
// // Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CONSOLE_CONTROLLER_HPP
#define CONSOLE_CONTROLLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <windows.h>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

namespace ma 
{
  class console_controller : private boost::noncopyable
  {
  public:
    typedef boost::function<void (void)> ctrl_function_type;

    console_controller(const ctrl_function_type& crl_function);    
    ~console_controller();    

  private:    
    static BOOL WINAPI console_ctrl_proc(DWORD ctrl_type);
    static boost::mutex ctrl_mutex_;
    static ctrl_function_type ctrl_function_;	
  }; // class console_controller

} // namespace ma

#endif // CONSOLE_CONTROLLER_HPP