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
#include <boost/throw_exception.hpp>
#include <QtCore/QMetaType>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/qt/meta_type_register_error.h>
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
  namespace 
  {
    template <typename Type>
    void registerQtMetaType(const char* typeName)
    {
      int typeId = qRegisterMetaType<Type>(typeName);
      if (!QMetaType::isRegistered(typeId))
      {
        boost::throw_exception(meta_type_register_error(typeName));
      }      
    } // registerQtMetaType

    void registerBoostSystemErrorCodeMetaType()
    {      
      registerQtMetaType<boost::system::error_code>("boost::system::error_code");
    } // registerBoostSystemErrorCodeMetaType

  } // namespace

  void registerMetaTypes()
  {
    registerBoostSystemErrorCodeMetaType();
  } // registerMetaTypes

  SessionManagerWrapper::SessionManagerWrapper(
    boost::asio::io_service& io_service, 
    boost::asio::io_service& session_io_service, 
    const ma::echo::server::session_manager_config& config,
    QObject* parent)
    : QObject(parent) 
    , sessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
    , sessionManager_(boost::make_shared<ma::echo::server::session_manager>(
        boost::ref(io_service), boost::ref(session_io_service), config))
    , startInProgress_(false)
    , waitInProgress_(false)
    , stopInProgress_(false)
  {
    connect(sessionManagerSignal_.get(), 
      SIGNAL(startComplete(const boost::system::error_code&)), 
      SLOT(onStartComplete(const boost::system::error_code&)),
      Qt::QueuedConnection);
    connect(sessionManagerSignal_.get(), 
      SIGNAL(waitComplete(const boost::system::error_code&)), 
      SLOT(onWaitComplete(const boost::system::error_code&)),
      Qt::QueuedConnection);
    connect(sessionManagerSignal_.get(), 
      SIGNAL(stopComplete(const boost::system::error_code&)), 
      SLOT(onStopComplete(const boost::system::error_code&)),
      Qt::QueuedConnection);
  } // SessionManagerWrapper::SessionManagerWrapper 

  SessionManagerWrapper::~SessionManagerWrapper()
  {
  } // SessionManagerWrapper::~SessionManagerWrapper

  void SessionManagerWrapper::asyncStart(boost::system::error_code& error)
  {
    error = ma::echo::server::server_error::invalid_state;
    if (startInProgress_)
    {
      //todo
      return;
    }
    //todo    
  } // SessionManagerWrapper::asyncStart

  void SessionManagerWrapper::asyncWait(boost::system::error_code& error)
  {    
    error = ma::echo::server::server_error::invalid_state;
    if (waitInProgress_)
    {
      //todo
      return;
    }
    //todo    
  } // SessionManagerWrapper::asyncWait

  void SessionManagerWrapper::asyncStop(boost::system::error_code& error)
  {    
    error = ma::echo::server::server_error::invalid_state;
    if (startInProgress_)
    {
      //todo
      return;
    }
    //todo    
  } // SessionManagerWrapper::asyncStop

  void SessionManagerWrapper::onStartComplete(const boost::system::error_code& /*error*/)
  {    
    startInProgress_ = false;
    //todo
  } // SessionManagerWrapper::onStartComplete

  void SessionManagerWrapper::onWaitComplete(const boost::system::error_code& /*error*/)
  {    
    waitInProgress_ = false;
    //todo
  } // SessionManagerWrapper::onWaitComplete

  void SessionManagerWrapper::onStopComplete(const boost::system::error_code& /*error*/)
  {    
    stopInProgress_ = false;
    //todo
  } // SessionManagerWrapper::onStopComplete

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma