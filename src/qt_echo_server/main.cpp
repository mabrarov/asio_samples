//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <memory>
#include <QtCore/QLocale>
#include <QtCore/QString>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include <ma/echo/server/qt/service.h>
#include <ma/echo/server/qt/custommetatypes.h>
#include <ma/echo/server/qt/mainform.h>

namespace
{
  void installTranslation(QCoreApplication& application, const QString& path,
    const QString& localeName, const QString& baseFilename)
  {
    std::auto_ptr<QTranslator> translator(new QTranslator(&application));
    if (translator->load(baseFilename + localeName, path))
    {
      application.installTranslator(translator.get());
      translator.release();
    }
  }

  void setUpTranslation(QApplication& application)
  {
    QString translationPath(QString::fromUtf8(":/ma/echo/server/qt/translation/"));
    QString sysLocaleName(QLocale::system().name());    
    installTranslation(application, translationPath, sysLocaleName, QString::fromUtf8("qt_"));
    installTranslation(application, translationPath, sysLocaleName, QString::fromUtf8("qt_echo_server_"));
  }

} // namespace

int main(int argc, char* argv[])
{
  QApplication application(argc, argv);

#pragma warning(push)
#pragma warning(disable: 4127)
  Q_INIT_RESOURCE(qt_echo_server);
#pragma warning(pop)
  
  setUpTranslation(application);

  using namespace ma::echo::server::qt;
  registerCustomMetaTypes();
  Service echoService;
  MainForm mainForm(echoService);
  mainForm.show();

  // Run application event loop
  return application.exec();
} // main
