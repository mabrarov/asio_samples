#
# Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       -= core gui
TARGET    = windows_console_signal_test
CONFIG   += console thread
CONFIG   -= app_bundle

# Common project configuration
include(../config.pri)

HEADERS  += ../../../include/ma/detail/binder.hpp \
            ../../../include/ma/detail/handler_ptr.hpp \
            ../../../include/ma/detail/intrusive_list.hpp \
            ../../../include/ma/detail/service_base.hpp \
            ../../../include/ma/detail/sp_singleton.hpp \
            ../../../include/ma/windows/console_signal.hpp \
            ../../../include/ma/windows/console_signal_service.hpp \
            ../../../include/ma/bind_handler.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/handler_cont_helpers.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/type_traits.hpp

SOURCES  += ../../../src/ma/windows/console_signal_service.cpp \
            ../../../src/windows_console_signal_test/main.cpp

INCLUDEPATH += $${BOOST_INCLUDE} \
               ../../../include

LIBS       += -L$${BOOST_LIB}
unix:LIBS  += $${BOOST_LIB}/libboost_system.a \
              $${BOOST_LIB}/libboost_thread.a \
              $${BOOST_LIB}/libboost_date_time.a
exists($${BOOST_INCLUDE}/boost/chrono.hpp) {
  unix:LIBS += $${BOOST_LIB}/libboost_chrono.a \
               -lrt
}

win32:DEFINES += WINVER=0x0500 \
                 _WIN32_WINNT=0x0500
