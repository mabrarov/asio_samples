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

#include <cstddef>
#include <boost/scoped_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <QtCore/QObject>
#include <ma/echo/server/session_manager_config_fwd.hpp>
#include <ma/echo/server/qt/forwardservicesignal.h>
#include <ma/echo/server/qt/service_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        struct execution_config
        {
          std::size_t session_manager_thread_count;
          std::size_t session_thread_count;          

          execution_config(
            std::size_t the_session_manager_thread_count, 
            std::size_t the_session_thread_count);
        }; // struct execution_config

        class Service : public QObject
        {
          Q_OBJECT

        public:
          explicit Service(QObject* parent = 0);
          ~Service();          

          void asyncStart(const execution_config&, const session_manager_config&);

        public slots:
          void asyncStop();
          void terminateWork();

        signals:
          void workException();
          void startComplete(const boost::system::error_code& error);
          void stopComplete(const boost::system::error_code& error);
          void workComplete(const boost::system::error_code& error);

        private slots:
          void onWorkException();
          void onStartComplete(const boost::system::error_code& error);
          void onWaitComplete(const boost::system::error_code& error);
          void onStopComplete(const boost::system::error_code& error);          

        private:
          Q_DISABLE_COPY(Service)

          class Work;
          bool isActualSignalSender(QObject* sender) const;
          boost::scoped_ptr<Work> work_;       
          ForwardServiceSignal forwardSignal_;
        }; // class Service

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICE_H
