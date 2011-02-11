//
// // Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <ma/console_controller.hpp>

namespace ma
{
  boost::mutex console_controller::ctrl_mutex_;
  console_controller::ctrl_function_type console_controller::ctrl_function_;

  console_controller::console_controller(const ctrl_function_type& crl_function)
  {
    boost::mutex::scoped_lock lock(ctrl_mutex_);
    if (ctrl_function_)
    {
      boost::throw_exception(std::logic_error("console_controller must be the only"));
    }
    ctrl_function_ = crl_function;
    lock.unlock();
    ::SetConsoleCtrlHandler(console_ctrl_proc, TRUE);
  }

  console_controller::~console_controller()
  {
    ::SetConsoleCtrlHandler(console_ctrl_proc, FALSE);
    boost::mutex::scoped_lock lock(ctrl_mutex_);
    ctrl_function_.clear();    
  }

  BOOL WINAPI console_controller::console_ctrl_proc(DWORD ctrl_type)
  {    
    boost::mutex::scoped_lock lock(ctrl_mutex_);
    switch (ctrl_type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:            
      ctrl_function_();
      return TRUE;
    default:
      return FALSE;
    }
  }
} // namespace ma
