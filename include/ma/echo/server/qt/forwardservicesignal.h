//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_FORWARDSERVICESIGNAL_H
#define MA_ECHO_SERVER_QT_FORWARDSERVICESIGNAL_H

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
        class ForwardServiceSignal : public QObject
        {
          Q_OBJECT

        private:
          typedef boost::recursive_mutex mutex_type;

        signals:          
          void startComplete(const boost::system::error_code&);
          void stopComplete(const boost::system::error_code&);

        public:
          ForwardServiceSignal()
          {
          }

          ~ForwardServiceSignal()
          {
          }

          void emitStartComplete(const boost::system::error_code& error)
          {
            emit startComplete(error);
          }

          void emitStopComplete(const boost::system::error_code& error)
          {
            emit stopComplete(error);
          }
               
        private:          
          Q_DISABLE_COPY(ForwardServiceSignal)
        }; // class ForwardServiceSignal

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_FORWARDSERVICESIGNAL_H
