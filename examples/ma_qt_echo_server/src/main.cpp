//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <exception>
#include <qglobal.h>
#include <QApplication>

#if defined(QT_STATIC) || !(defined(QT_DLL) || defined(QT_SHARED))
#if QT_VERSION >= 0x050000
#include <QtPlugin>
#endif
#endif

#include "service.h"
#include "custommetatypes.h"
#include "mainform.h"

#if defined(QT_STATIC) || !(defined(QT_DLL) || defined(QT_SHARED))
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050F00
#if defined(WIN32)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#else
// todo: add support for the rest of platforms supported by Qt - 
// import platform specific plugin with Q_IMPORT_PLUGIN macro.
#error Platform specific Qt Platform Integration plugin should be imported
#endif
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
    qFatal("Unexpected error: %s", e.what());
  }
  catch (...)
  {
    qFatal("Unknown exception");
  }
}
