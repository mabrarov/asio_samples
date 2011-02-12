//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SESSIONMANAGERWRAPPER_H
#define MA_ECHO_SERVER_QT_SESSIONMANAGERWRAPPER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <QtCore/QObject>
#include <ma/echo/server/session_manager_fwd.hpp>
#include <ma/echo/server/session_manager_config_fwd.hpp>
#include <ma/echo/server/qt/sessionmanagersignal_fwd.h>
#include <ma/echo/server/qt/sessionmanagerwrapper_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        class SessionManagerWrapper : public QObject
        {
          Q_OBJECT

        public:
          SessionManagerWrapper(
            boost::asio::io_service& io_service, 
            boost::asio::io_service& session_io_service, 
            const ma::echo::server::session_manager_config& config,
            QObject* parent = 0);

          ~SessionManagerWrapper();
        
          void asyncStart();
          void asyncWait();
          void asyncStop();

        signals:
          void startComplete(const boost::system::error_code& error);
          void waitComplete(const boost::system::error_code& error);
          void stopComplete(const boost::system::error_code& error);        

        private:          
          Q_DISABLE_COPY(SessionManagerWrapper)
                    
          SessionManagerSignalPtr sessionManagerSignal_;
          ma::echo::server::session_manager_ptr sessionManager_;
        }; // class SessionManagerWrapper

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SESSIONMANAGERWRAPPER_H
