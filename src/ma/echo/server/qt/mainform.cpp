/*
TRANSLATOR ma::echo::server::qt::MainForm
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/server/qt/sessionmanagerwrapper.h>
#include <ma/echo/server/qt/mainform.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{
  MainForm::MainForm(QWidget* parent, Qt::WFlags flags)
    : QDialog(parent, flags | Qt::WindowMinimizeButtonHint)
  {
    ui_.setupUi(this);    
    //todo    
  }

  MainForm::~MainForm()
  {
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
