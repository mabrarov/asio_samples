//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SESSIONMANAGERSIGNAL_H
#define MA_ECHO_SERVER_QT_SESSIONMANAGERSIGNAL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/thread/mutex.hpp>
#include <QtCore/QObject>
#include <ma/echo/server/qt/sessionmanagersignal_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {        
        class SessionManagerSignal : public QObject
        {
          Q_OBJECT

        public:
          SessionManagerSignal()
          {
          }

          ~SessionManagerSignal()
          {
          }

        signals:
          void startComplete(const boost::system::error_code& error);
          void waitComplete(const boost::system::error_code& error);
          void stopComplete(const boost::system::error_code& error);        

        public:
          void emitStartComplete(const boost::system::error_code& error)
          {
            mutex_type::scoped_lock lock(mutex_);
            emit startComplete(error);
          }

          void emitWaitComplete(const boost::system::error_code& error)
          {
            mutex_type::scoped_lock lock(mutex_);
            emit waitComplete(error);
          }

          void emitStopComplete(const boost::system::error_code& error)
          {
            mutex_type::scoped_lock lock(mutex_);
            emit stopComplete(error);
          }

        private:
          typedef boost::mutex mutex_type;

          Q_DISABLE_COPY(SessionManagerSignal)          

          mutex_type mutex_;
        }; // SessionManagerSignal
        
      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SESSIONMANAGERSIGNAL_H
