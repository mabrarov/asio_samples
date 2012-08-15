#
# Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       -= core gui
TARGET    = async_basics
CONFIG   += console thread
CONFIG   -= app_bundle

# Common project configuration
include(../config.pri)

HEADERS  += ../../../include/ma/handler_storage_service.hpp \
            ../../../include/ma/handler_storage.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/context_alloc_handler.hpp \
            ../../../include/ma/context_wrapped_handler.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/strand_wrapped_handler.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/console_controller.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/type_traits.hpp \
            ../../../include/ma/shared_ptr_factory.hpp \
            ../../../include/ma/steady_deadline_timer.hpp \
            ../../../include/ma/tutorial/async_interface.hpp \
            ../../../include/ma/tutorial/async_implementation.hpp

SOURCES  += ../../../src/ma/console_controller.cpp \
            ../../../src/async_basics/main.cpp \
            ../../../src/ma/tutorial/async_implementation.cpp

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

win32:DEFINES += WIN32_LEAN_AND_MEAN \
                 _UNICODE \
                 UNICODE \
                 WINVER=0x0500 \
                 _WIN32_WINNT=0x0500 \
                 _WIN32_WINDOWS=0x0410 \
                 _WIN32_IE=0x0600

linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -std=c++0x \
                    -Wstrict-aliasing
}
