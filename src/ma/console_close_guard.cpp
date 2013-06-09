//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//#if defined(WIN32)
//#include <windows.h>
//#endif

//#if !defined(WIN32)
//#include <csignal>
//#endif

#include <csignal>
//#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
//#include <boost/thread/locks.hpp>
//#include <boost/thread/mutex.hpp>
#include <ma/console_close_guard.hpp>

namespace {

class console_close_guard_base_0 : private boost::noncopyable
{
public:
  console_close_guard_base_0()
    : io_service_()
  {
  }

protected:
  ~console_close_guard_base_0()
  {
  }

  boost::asio::io_service io_service_;
}; // console_close_guard_base_0

class console_close_guard_base_1 : public console_close_guard_base_0
{
public:
  console_close_guard_base_1()
    : console_close_guard_base_0()
    , work_guard_(io_service_)
  {
  }

protected:
  ~console_close_guard_base_1()
  {
  }

  boost::asio::io_service::work work_guard_;
}; // console_close_guard_base_1

class console_close_guard_base_2 : public console_close_guard_base_1
{
public:
  console_close_guard_base_2()
    : console_close_guard_base_1()
    , work_thread_(boost::bind(&boost::asio::io_service::run, 
          boost::ref(io_service_)))
  {
  }

protected:
  ~console_close_guard_base_2()
  {
    io_service_.stop();
    work_thread_.join();
  }

  boost::thread work_thread_;
}; // console_close_guard_base_2

} // anonymous namespace

namespace ma {

class console_close_guard::implementation : private console_close_guard_base_2
{
public:
  implementation(const ctrl_function_type& ctrl_function)
    : console_close_guard_base_2()
    , signal_set_(io_service_, SIGINT)
  {
    signal_set_.async_wait(boost::bind(&handle_signal, _1, ctrl_function));
  }

  ~implementation()
  {
    boost::system::error_code ignored;
    signal_set_.cancel(ignored);
  }  
  
private:
  static void handle_signal(const boost::system::error_code& error,
      ctrl_function_type ctrl_function)
  {
    if (boost::asio::error::operation_aborted != error)
    {
      ctrl_function();
    }
  }

  boost::asio::signal_set signal_set_;
}; // class console_close_guard::implementation

console_close_guard::console_close_guard(
    const ctrl_function_type& ctrl_function)
  : implementation_(new implementation(ctrl_function))
{
}

console_close_guard::~console_close_guard()
{
}

//#if defined(WIN32)
//  static BOOL WINAPI win_console_ctrl_routine(DWORD ctrl_type)
//  {
//    switch (ctrl_type)
//    {
//    case CTRL_C_EVENT:
//    case CTRL_BREAK_EVENT:
//    case CTRL_CLOSE_EVENT:
//    case CTRL_SHUTDOWN_EVENT:
//    case CTRL_LOGOFF_EVENT:
//      console_close_guard::get_registry()->handle_console_close();
//      return FALSE;
//    default:
//      return FALSE;
//    }
//  }
//#endif

//console_close_guard::console_close_guard(const ctrl_function_type& crl_function)
//{
//  {
//    lock_guard_type lock(ctrl_mutex_);
//    if (ctrl_function_)
//    {
//      boost::throw_exception(std::logic_error(
//          "console_close_guard must be the only"));
//    }
//    ctrl_function_ = crl_function;
//  }
//
//#if defined(WIN32)
//  ::SetConsoleCtrlHandler(console_ctrl_proc, TRUE);
//#else
//  if (SIG_ERR == ::signal(SIGINT, &console_close_guard::console_ctrl_proc))
//  {
//    boost::throw_exception(std::runtime_error(
//        "failed to set signal handler for SIGQUIT"));
//  }
//#endif // defined(WIN32)
//}
//
//console_close_guard::~console_close_guard()
//{
//#if defined(WIN32)
//  ::SetConsoleCtrlHandler(console_ctrl_proc, FALSE);
//#endif
//
//  lock_guard_type lock(ctrl_mutex_);
//  ctrl_function_.clear();
//}

} // namespace ma
