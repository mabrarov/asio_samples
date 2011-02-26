//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if !defined(WIN32)
#include <csignal> 
#endif

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <ma/console_controller.hpp>

namespace ma
{
  console_controller::mutex_type console_controller::ctrl_mutex_;
  console_controller::ctrl_function_type console_controller::ctrl_function_;

  console_controller::console_controller(const ctrl_function_type& crl_function)
  {
    mutex_type::scoped_lock lock(ctrl_mutex_);
    if (ctrl_function_)
    {
      boost::throw_exception(std::logic_error("console_controller must be the only"));
    }
    ctrl_function_ = crl_function;
    lock.unlock();
#if defined(WIN32)
    ::SetConsoleCtrlHandler(console_ctrl_proc, TRUE);
#else
    if (SIG_ERR == ::signal(SIGINT, &console_controller::console_ctrl_proc))
    {
      boost::throw_exception(std::runtime_error("failed to set signal handler for SIGQUIT"));
    }
#endif
  }

  console_controller::~console_controller()
  {
#if defined(WIN32)
    ::SetConsoleCtrlHandler(console_ctrl_proc, FALSE);
#endif
    mutex_type::scoped_lock lock(ctrl_mutex_);
    ctrl_function_.clear();    
  }

#if defined(WIN32)
  BOOL WINAPI console_controller::console_ctrl_proc(DWORD ctrl_type)
  {    
    mutex_type::scoped_lock lock(ctrl_mutex_);
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
#else
  void console_controller::console_ctrl_proc(int /*signal*/)
  {
    mutex_type::scoped_lock lock(ctrl_mutex_);
    if (!ctrl_function_.empty())
    {
      ctrl_function_();
    }
    (void) ::signal(SIGINT, SIG_DFL);
  }
#endif // defined(WIN32)

} // namespace ma
