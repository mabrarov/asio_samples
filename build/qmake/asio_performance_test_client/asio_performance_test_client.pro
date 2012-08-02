#
# Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       -= core gui
TARGET    = asio_performance_test_client
CONFIG   += console thread
CONFIG   -= app_bundle

# Common project configuration
include(../config.pri)

HEADERS  += ../../../include/ma/async_connect.hpp \
            ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/context_wrapped_handler.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/cyclic_buffer.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/steady_deadline_timer.hpp \
            ../../../include/ma/strand_wrapped_handler.hpp \
            ../../../include/ma/type_traits.hpp

SOURCES  += ../../../src/asio_performance_test_client/main.cpp

INCLUDEPATH += $${BOOST_INCLUDE} \
               ../../../include

LIBS       += -L$${BOOST_LIB}
unix:LIBS  += $${BOOST_LIB}/libboost_system.a \
              $${BOOST_LIB}/libboost_thread.a \
              $${BOOST_LIB}/libboost_date_time.a \
              $${BOOST_LIB}/libboost_program_options.a
exists($${BOOST_INCLUDE}/boost/chrono.hpp) {
  unix:LIBS += $${BOOST_LIB}/libboost_chrono.a \
               -lrt
}
exists($${BOOST_INCLUDE}/boost/timer/timer.hpp) {
  unix:LIBS += $${BOOST_LIB}/libboost_timer.a
}

win32:DEFINES += WIN32_LEAN_AND_MEAN \
                 _UNICODE \
                 UNICODE \
                 WINVER=0x0501 \
                 _WIN32_WINNT=0x0501

linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -std=c++0x \
                    -Wstrict-aliasing
}
