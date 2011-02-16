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

#include <QtGui/QDialog>
#include <ma/echo/server/qt/sessionmanagerwrapper_fwd.h>
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
          MainForm(QWidget* parent = 0, Qt::WFlags flags = 0);
          ~MainForm();

        private:
          Q_DISABLE_COPY(MainForm) 

          Ui::mainForm ui_;          
        }; // class MainForm

      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_MAINFORM_H
