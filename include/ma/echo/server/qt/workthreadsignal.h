//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_WORKTHREADSIGNAL_H
#define MA_ECHO_SERVER_QT_WORKTHREADSIGNAL_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

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
        class WorkThreadSignal : public QObject
        {
          Q_OBJECT

        private:
          typedef boost::recursive_mutex mutex_type;

        public:
          WorkThreadSignal()
            : mutex_()
          {
          }

          ~WorkThreadSignal()
          {
          }

        signals:
          void workException();          

        public:
          void emitWorkException()
          {           
            mutex_type::scoped_lock lock(mutex_);
            emit workException();
          }          

        private:          
          Q_DISABLE_COPY(WorkThreadSignal)

          mutex_type mutex_;
        }; // class WorkThreadSignal

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_WORKTHREADSIGNAL_H
