//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if !defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
#include <csignal>
#endif

#include <boost/config.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <ma/console_close_guard.hpp>
#include <ma/windows/console_signal.hpp>

namespace {

class console_close_guard_base_0 : private boost::noncopyable
{
public:
  console_close_guard_base_0()
    : io_service_(1)
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
  typedef ma::console_close_guard::ctrl_function_type ctrl_function_type;

  explicit console_close_guard_base_1(const ctrl_function_type& ctrl_function)
    : console_close_guard_base_0()
#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
    , signal_alerter_(io_service_)
#else
    , signal_alerter_(io_service_, SIGINT, SIGTERM)
#endif
  {
#if !defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL) && defined(SIGQUIT)
    signal_alerter_.add(SIGQUIT);
#endif
    start_wait(signal_alerter_, ctrl_function);
  }

protected:
  ~console_close_guard_base_1()
  {
  }

private:

#if defined(MA_HAS_WINDOWS_CONSOLE_SIGNAL)
  typedef ma::windows::console_signal signal_alerter;
#else
  typedef boost::asio::signal_set     signal_alerter;
#endif

  static void handle_signal(const boost::system::error_code& error,
      signal_alerter& alerter, ctrl_function_type ctrl_function)
  {
    if (boost::asio::error::operation_aborted != error)
    {
      start_wait(alerter, ctrl_function);
      ctrl_function();
    }
  }

  static void start_wait(signal_alerter& alerter, 
      const ctrl_function_type& ctrl_function)
  {
    alerter.async_wait(
        boost::bind(&handle_signal, _1, boost::ref(alerter), ctrl_function));
  }

  signal_alerter signal_alerter_;
}; // console_close_guard_base_1

} // anonymous namespace

namespace ma {

class console_close_guard::implementation : private console_close_guard_base_1
{
public:
  explicit implementation(const ctrl_function_type& ctrl_function)
    : console_close_guard_base_1(ctrl_function)
    , work_thread_(boost::bind(&boost::asio::io_service::run, 
          boost::ref(io_service_)))
  {    
  }

  ~implementation()
  {
    io_service_.stop();
    work_thread_.join();
  }
  
private:
  boost::thread work_thread_;
}; // class console_close_guard::implementation

console_close_guard::console_close_guard(
    const ctrl_function_type& ctrl_function)
  : implementation_(new implementation(ctrl_function))
{
}

console_close_guard::~console_close_guard()
{
}

} // namespace ma
