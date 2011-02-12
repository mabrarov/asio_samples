/*
TRANSLATOR ma::echo::server::qt::SessionManagerWrapper
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/sessionmanagersignal.h>
#include <ma/echo/server/qt/sessionmanagerwrapper.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  SessionManagerWrapper::SessionManagerWrapper(
    boost::asio::io_service& io_service, 
    boost::asio::io_service& session_io_service, 
    const ma::echo::server::session_manager_config& config,
    QObject* parent)
    : QObject(parent) 
    , sessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
    , sessionManager_(boost::make_shared<ma::echo::server::session_manager>(
        boost::ref(io_service), boost::ref(session_io_service), config))
  {
    checkConnect(connect(sessionManagerSignal_.get(), 
      SIGNAL(startComplete(const boost::system::error_code&)), 
      SIGNAL(startComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(connect(sessionManagerSignal_.get(), 
      SIGNAL(waitComplete(const boost::system::error_code&)), 
      SIGNAL(waitComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(connect(sessionManagerSignal_.get(), 
      SIGNAL(stopComplete(const boost::system::error_code&)), 
      SIGNAL(stopComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
  } // SessionManagerWrapper::SessionManagerWrapper 

  SessionManagerWrapper::~SessionManagerWrapper()
  {
  } // SessionManagerWrapper::~SessionManagerWrapper

  void SessionManagerWrapper::asyncStart()
  {        
    sessionManager_->async_start(boost::bind(
      &SessionManagerSignal::emitStartComplete, sessionManagerSignal_, _1));    
  } // SessionManagerWrapper::asyncStart

  void SessionManagerWrapper::asyncWait()
  {    
    sessionManager_->async_wait(boost::bind(
      &SessionManagerSignal::emitWaitComplete, sessionManagerSignal_, _1));    
  } // SessionManagerWrapper::asyncWait

  void SessionManagerWrapper::asyncStop()
  {    
    sessionManager_->async_stop(boost::bind(
      &SessionManagerSignal::emitStopComplete, sessionManagerSignal_,_1));    
  } // SessionManagerWrapper::asyncStop  

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma