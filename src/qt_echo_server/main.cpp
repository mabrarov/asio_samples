//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <QtGui/QApplication>
#include <ma/echo/server/qt/service.h>
#include <ma/echo/server/qt/custommetatypes.h>
#include <ma/echo/server/qt/mainform.h>

int main(int argc, char* argv[])
{
  QApplication application(argc, argv);

  using namespace ma::echo::server::qt;
  registerCustomMetaTypes();
  Service echoService;
  MainForm mainForm(echoService);
  mainForm.show();

  return application.exec();
}
