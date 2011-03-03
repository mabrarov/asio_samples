//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_MAINFORM_H
#define MA_ECHO_SERVER_QT_MAINFORM_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <QtGui/QDialog>
#include <ma/echo/server/qt/service_fwd.h>
#include <ui_mainform.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        class MainForm : public QDialog
        {
          Q_OBJECT

        public:
          MainForm(Service& echoService, QWidget* parent = 0, Qt::WFlags flags = 0);
          ~MainForm();

        private slots:      
           void on_startButton_clicked();
           void on_stopButton_clicked();
           void on_terminateButton_clicked();

           void on_echoService_workException();
           void on_echoService_startComplete(const boost::system::error_code&);
           void on_echoService_stopComplete(const boost::system::error_code&);
           void on_echoService_workComplete(const boost::system::error_code&);           

        private:
          Q_DISABLE_COPY(MainForm) 

          void writeLog(const QString&);

          Ui::mainForm ui_;
          Service& echoService_;
        }; // class MainForm

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_MAINFORM_H
