//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_SERVICE_H
#define MA_ECHO_SERVER_QT_SERVICE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <QObject>
#include <ma/echo/server/session_manager_config_fwd.hpp>
#include <ma/echo/server/session_manager_stats.hpp>
#include <ma/detail/memory.hpp>
#include "servicestate.h"
#include "execution_config_fwd.h"
#include "serviceforwardsignal_fwd.h"
#include "serviceservantsignal_fwd.h"
#include "service_fwd.h"

namespace ma {
namespace echo {
namespace server {
namespace qt {

class Service : public QObject
{
  Q_OBJECT //-V524

public:
  explicit Service(QObject* parent = 0);
  ~Service();

  ServiceState::State state() const;
  session_manager_stats stats() const;

  void asyncStart(const execution_config&, const session_manager_config&);

public slots:
  void asyncStop();
  void terminate();

signals:
  void exceptionHappened();
  void startCompleted(const boost::system::error_code&);
  void stopCompleted(const boost::system::error_code&);
  void workCompleted(const boost::system::error_code&);

private slots:
  void onServantWorkThreadExceptionHappened();
  void onServantStartCompleted(const boost::system::error_code&);
  void onServantWaitCompleted(const boost::system::error_code&);
  void onServantStopCompleted(const boost::system::error_code&);

private:
  class server;

  Q_DISABLE_COPY(Service)

  void createServant(const execution_config&, const session_manager_config&);
  void destroyServant();

  ServiceState::State   state_;
  ServiceForwardSignal* forwardSignal_;
  session_manager_stats stats_;

#if defined(MA_USE_CXX11_STDLIB_MEMORY)
  detail::unique_ptr<server> server_;
#else
  detail::scoped_ptr<server> server_;
#endif

  detail::shared_ptr<ServiceServantSignal> servantSignal_;
}; // class Service

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

inline ma::echo::server::qt::ServiceState::State 
ma::echo::server::qt::Service::state() const
{
  return state_;
}

#endif // MA_ECHO_SERVER_QT_SERVICE_H
