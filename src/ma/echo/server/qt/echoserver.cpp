/*
TRANSLATOR ma::echo::server::qt::EchoServer
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/noncopyable.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/qt/sessionmanagerwrapper.h>
#include <ma/echo/server/qt/echoserver.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  struct EchoServer::Service : private boost::noncopyable
  {
 
  }; // struct EchoServer::Service

  EchoServer::EchoServer(QObject* parent)
    : QObject(parent)
    , service_()
  {
  } // EchoServer::EchoServer

  EchoServer::~EchoServer()
  {
    
  } // EchoServer::~EchoServer

  void EchoServer::asyncStart()
  {
    boost::system::error_code error;
    if (service_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit startComplete(error);
      return;
    }
    //todo
  } // EchoServer::asyncStart

  void EchoServer::asyncWait()
  {
    boost::system::error_code error;
    if (!service_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit waitComplete(error);
      return;
    }
    //todo
  } // EchoServer::asyncWait
  
  void EchoServer::asyncStop()
  {
    boost::system::error_code error;
    if (!service_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
      return;
    }
    //todo
  } // EchoServer::asyncStop

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
