//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <exception>
#include <qglobal.h>
#include <QApplication>

#if defined(QT_STATIC) || !(defined(QT_DLL) || defined(QT_SHARED))
#if QT_VERSION >= 0x050000
#include <QtPlugin>
#endif
#endif

#include <ma/echo/server/qt/service.h>
#include <ma/echo/server/qt/custommetatypes.h>
#include <ma/echo/server/qt/mainform.h>

#if defined(QT_STATIC) || !(defined(QT_DLL) || defined(QT_SHARED))
#if QT_VERSION >= 0x050000
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
#endif

int main(int argc, char* argv[])
{
  try
  {
    QApplication application(argc, argv);

    using namespace ma::echo::server::qt;
    registerCustomMetaTypes();
    Service echoService;
    MainForm mainForm(echoService);
    mainForm.show();

    return application.exec();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
}
