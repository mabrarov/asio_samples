//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICEFORWARDSIGNAL_H
#define MA_ECHO_SERVER_QT_SERVICEFORWARDSIGNAL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <QtCore/QObject>
#include <ma/echo/server/qt/serviceforwardsignal_fwd.h>

namespace ma {

namespace echo {

namespace server {

namespace qt {

class ServiceForwardSignal : public QObject
{
  Q_OBJECT

private:
  typedef boost::recursive_mutex mutex_type;

signals:          
  void startCompleted(const boost::system::error_code&);
  void stopCompleted(const boost::system::error_code&);
  void workCompleted(const boost::system::error_code&);

public:
  ServiceForwardSignal(QObject* parent = 0)
    : QObject(parent)
  {
  }

  ~ServiceForwardSignal()
  {
  }

  void emitStartCompleted(const boost::system::error_code& error)
  {
    emit startCompleted(error);
  }

  void emitStopCompleted(const boost::system::error_code& error)
  {
    emit stopCompleted(error);
  }

  void emitWorkCompleted(const boost::system::error_code& error)
  {
    emit workCompleted(error);
  }
               
private:          
  Q_DISABLE_COPY(ServiceForwardSignal)
}; // class ServiceForwardSignal

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICEFORWARDSIGNAL_H
