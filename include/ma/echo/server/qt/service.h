//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICE_H
#define MA_ECHO_SERVER_QT_SERVICE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <QtCore/QObject>
#include <ma/echo/server/session_manager_options_fwd.hpp>
#include <ma/echo/server/qt/servicestate.h>
#include <ma/echo/server/qt/execution_options_fwd.h>
#include <ma/echo/server/qt/serviceforwardsignal_fwd.h>
#include <ma/echo/server/qt/serviceservantsignal_fwd.h>
#include <ma/echo/server/qt/service_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {        
        class Service : public QObject
        {
          Q_OBJECT

        public:          
          explicit Service(QObject* parent = 0);
          ~Service();
          void asyncStart(const execution_options&, const session_manager_options&);

          ServiceState::State currentState()
          {
            return currentState_;
          }

        public slots:
          void asyncStop();
          void terminate();

        signals:
          void exceptionHappened();
          void startCompleted(const boost::system::error_code&);
          void stopCompleted(const boost::system::error_code&);
          void workCompleted(const boost::system::error_code&);

        private slots:
          void onWorkThreadExceptionHappened();
          void onSessionManagerStartCompleted(const boost::system::error_code&);
          void onSessionManagerWaitCompleted(const boost::system::error_code&);
          void onSessionManagerStopCompleted(const boost::system::error_code&);

        private:
          class servant;

          Q_DISABLE_COPY(Service) 

          void createServant(const execution_options&, const session_manager_options&);
          void destroyServant();

          ServiceState::State currentState_;
          ServiceForwardSignal* forwardSignal_;          
          boost::scoped_ptr<servant> servant_;
          boost::shared_ptr<ServiceServantSignal> servantSignal_;
        }; // class Service

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICE_H
