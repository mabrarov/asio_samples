#
# Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE =  app
QT       -= core gui
TARGET   =  async_connect
CONFIG   += console thread
CONFIG   -= app_bundle

HEADERS  += ../../../include/ma/async_connect.hpp \
            ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/context_wrapped_handler.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/strand_wrapped_handler.hpp \
            ../../../include/ma/type_traits.hpp

SOURCES  += ../../../src/async_connect/main.cpp

win32:INCLUDEPATH += ../../../../boost_1_48_0
unix:INCLUDEPATH  += /usr/local/include
INCLUDEPATH       += ../../../include

win32:LIBS += -L../../../../boost_1_48_0/lib/x86
unix:LIBS  += -lboost_system

win32:DEFINES += WIN32_LEAN_AND_MEAN _UNICODE UNICODE _WIN32_WINNT=0x0501

linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -std=c++0x -Wstrict-aliasing
}
