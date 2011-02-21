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
#include <stdexcept>
#include <boost/scoped_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <ma/echo/server/qt/sessionmanagerwrapper_fwd.h>
#include <ma/echo/server/qt/service_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        struct ExecutionConfig
        {
          std::size_t sessionManagerThreadCount;
          std::size_t sessionThreadCount;
          long stopTimeout;

          ExecutionConfig(
            std::size_t theSessionManagerThreadCount,
            std::size_t theSessionThreadCount, 
            long theStopTimeout)
            : sessionManagerThreadCount(theSessionManagerThreadCount)
            , sessionThreadCount(theSessionThreadCount)
            , stopTimeout(theStopTimeout)
          {
            if (theSessionManagerThreadCount < 1)
            {
              boost::throw_exception(std::invalid_argument(
                "theSessionManagerThreadCount must be >= 1"));
            }
            if (theSessionThreadCount < 1)
            {
              boost::throw_exception(std::invalid_argument(
                "theSessionThreadCount must be >= 1"));
            }
            if (theStopTimeout < 0)
            {
              boost::throw_exception(std::invalid_argument(
                "theStopTimeout must be >= 0"));
            }
          }
        }; // struct ExecutionConfig        

        class Service : public QObject
        {
          Q_OBJECT

        public:
          explicit Service(QObject* parent = 0);
          ~Service();

          void asyncStart();
          void asyncWait();
          void asyncStop();

        signals:
          void startComplete(const boost::system::error_code& error);
          void waitComplete(const boost::system::error_code& error);
          void stopComplete(const boost::system::error_code& error);

        private:
          struct Impl;

          Q_DISABLE_COPY(Service);          

          boost::scoped_ptr<Impl> impl_;    
        }; // class EchoServer

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_SERVICE_H
