//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICESERVANTSIGNAL_H
#define MA_ECHO_SERVER_QT_SERVICESERVANTSIGNAL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <QObject>
#include <ma/echo/server/qt/serviceservantsignal_fwd.h>

namespace ma {
namespace echo {
namespace server {
namespace qt {

class ServiceServantSignal : public QObject
{
  Q_OBJECT

private:
  typedef boost::recursive_mutex mutex_type;
  typedef boost::lock_guard<mutex_type> lock_guard;

public:
  ServiceServantSignal()
  {
  }

  ~ServiceServantSignal()
  {
  }

signals:
  void workThreadExceptionHappened();
  void startCompleted(const boost::system::error_code&);
  void waitCompleted(const boost::system::error_code&);
  void stopCompleted(const boost::system::error_code&);

public:
  void emitWorkThreadExceptionHappened()
  {
    lock_guard lock(mutex_);
    emit workThreadExceptionHappened();
  }

  void emitStartCompleted(const boost::system::error_code& error)
  {
    lock_guard lock(mutex_);
    emit startCompleted(error);
  }

  void emitWaitCompleted(const boost::system::error_code& error)
  {
    lock_guard lock(mutex_);
    emit waitCompleted(error);
  }

  void emitStopCompleted(const boost::system::error_code& error)
  {
    lock_guard lock(mutex_);
    emit stopCompleted(error);
  }

  void disconnect()
  {
    lock_guard lock(mutex_);
    QObject::disconnect();
  }

private:
  Q_DISABLE_COPY(ServiceServantSignal)

  mutex_type mutex_;
}; // class ServiceServantSignal

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICESERVANTSIGNAL_H
