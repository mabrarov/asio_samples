//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICEWORKSIGNAL_H
#define MA_ECHO_SERVER_QT_SERVICEWORKSIGNAL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <QtCore/QObject>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        class ServiceWorkSignal;
        typedef boost::shared_ptr<ServiceWorkSignal> ServiceWorkSignalPtr;

        class ServiceWorkSignal : public QObject
        {
          Q_OBJECT

        private:
          typedef boost::recursive_mutex mutex_type;

        public:
          ServiceWorkSignal()
          {
          }

          ~ServiceWorkSignal()
          {
          }

        signals:
          void workThreadExceptionHappened();
          void sessionManagerStartCompleted(const boost::system::error_code&);
          void sessionManagerWaitCompleted(const boost::system::error_code&);
          void sessionManagerStopCompleted(const boost::system::error_code&);

        public:
          void emitWorkThreadExceptionHappened()
          {           
            mutex_type::scoped_lock lock(mutex_);
            emit workThreadExceptionHappened();
          }

          void emitSessionManagerStartCompleted(const boost::system::error_code& error)
          {       
            mutex_type::scoped_lock lock(mutex_);
            emit sessionManagerStartCompleted(error);
          }

          void emitSessionManagerWaitCompleted(const boost::system::error_code& error)
          {       
            mutex_type::scoped_lock lock(mutex_);
            emit sessionManagerWaitCompleted(error);
          }

          void emitSessionManagerStopCompleted(const boost::system::error_code& error)
          {       
            mutex_type::scoped_lock lock(mutex_);
            emit sessionManagerStopCompleted(error);
          }

        private:          
          Q_DISABLE_COPY(ServiceWorkSignal)

          mutex_type mutex_;
        }; // class ServiceWorkSignal

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICEWORKSIGNAL_H
