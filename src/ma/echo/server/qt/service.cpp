/*
TRANSLATOR ma::echo::server::qt::Service
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
#include <ma/echo/server/qt/service.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  struct Service::Impl : private boost::noncopyable
  {
 
  }; // struct Service::Impl

  Service::Service(QObject* parent)
    : QObject(parent)
    , impl_()
  {
  } // Service::Service

  Service::~Service()
  {
    
  } // Service::~Service

  void Service::asyncStart()
  {
    boost::system::error_code error;
    if (impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit startComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit startComplete(error);
    }    
  } // Service::asyncStart

  void Service::asyncWait()
  {
    boost::system::error_code error;
    if (!impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit waitComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit waitComplete(error);
    }    
  } // Service::asyncWait
  
  void Service::asyncStop()
  {
    boost::system::error_code error;
    if (!impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
    }    
  } // Service::asyncStop

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
