//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/throw_exception.hpp>
#include <boost/system/error_code.hpp>
#include <QtCore/QMetaType>
#include <ma/echo/server/qt/meta_type_register_error.h>
#include <ma/echo/server/qt/custommetatypes.h>

namespace ma {

namespace echo {

namespace server {

namespace qt {

namespace {

template <typename Type>
void registerMetaType(const char* typeName)
{
  int typeId = qRegisterMetaType<Type>(typeName);
  if (!QMetaType::isRegistered(typeId))
  {
    boost::throw_exception(meta_type_register_error(typeName));
  }      
}

void registerBoostSystemErrorCodeMetaType()
{
  registerMetaType<boost::system::error_code>("boost::system::error_code");
}

} // anonymous namespace

void registerCustomMetaTypes()
{
  registerBoostSystemErrorCodeMetaType();
}
  
} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma