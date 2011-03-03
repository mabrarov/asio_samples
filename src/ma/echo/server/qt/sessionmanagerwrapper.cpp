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
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
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
    , startSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
    , waitSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
    , stopSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
    , sessionManager_(boost::make_shared<ma::echo::server::session_manager>(
        boost::ref(io_service), boost::ref(session_io_service), config))
  {
    checkConnect(QObject::connect(startSessionManagerSignal_.get(), 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SIGNAL(startComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(waitSessionManagerSignal_.get(), 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SIGNAL(waitComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(stopSessionManagerSignal_.get(), 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SIGNAL(stopComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
  }

  SessionManagerWrapper::~SessionManagerWrapper()
  {
  }

  void SessionManagerWrapper::asyncStart()
  {        
    sessionManager_->async_start(
      boost::bind(&SessionManagerSignal::emitOperationComplete, startSessionManagerSignal_, _1));
  }

  void SessionManagerWrapper::asyncWait()
  {    
    sessionManager_->async_wait(
      boost::bind(&SessionManagerSignal::emitOperationComplete, waitSessionManagerSignal_, _1));
  }

  void SessionManagerWrapper::asyncStop()
  {    
    sessionManager_->async_stop(
      boost::bind(&SessionManagerSignal::emitOperationComplete, stopSessionManagerSignal_,_1));
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma