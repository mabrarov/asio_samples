# 
# Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       += core gui
TARGET    = qt_echo_server
CONFIG   += qt thread

HEADERS  += ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/context_alloc_handler.hpp \
            ../../../include/ma/context_wrapped_handler.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/cyclic_buffer.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/strand_wrapped_handler.hpp \
            ../../../include/ma/type_traits.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/handler_storage.hpp \
            ../../../include/ma/handler_storage_service.hpp \
            ../../../include/ma/echo/server/error.hpp \
            ../../../include/ma/echo/server/session.hpp \
            ../../../include/ma/echo/server/session_config.hpp \
            ../../../include/ma/echo/server/session_config_fwd.hpp \
            ../../../include/ma/echo/server/session_fwd.hpp \
            ../../../include/ma/echo/server/session_manager.hpp \
            ../../../include/ma/echo/server/session_manager_config.hpp \
            ../../../include/ma/echo/server/session_manager_config_fwd.hpp \
            ../../../include/ma/echo/server/session_manager_fwd.hpp \
            ../../../include/ma/echo/server/qt/custommetatypes.h \
            ../../../include/ma/echo/server/qt/execution_config.h \
            ../../../include/ma/echo/server/qt/execution_config_fwd.h \
            ../../../include/ma/echo/server/qt/serviceforwardsignal_fwd.h \
            ../../../include/ma/echo/server/qt/serviceservantsignal_fwd.h \
            ../../../include/ma/echo/server/qt/servicestate.h \
            ../../../include/ma/echo/server/qt/service_fwd.h \
            ../../../include/ma/echo/server/qt/meta_type_register_error.h \
            ../../../include/ma/echo/server/qt/signal_connect_error.h \
            ../../../include/ma/echo/server/qt/service.h \
            ../../../include/ma/echo/server/qt/serviceforwardsignal.h \
            ../../../include/ma/echo/server/qt/serviceservantsignal.h \
            ../../../include/ma/echo/server/qt/mainform.h

SOURCES  += ../../../src/qt_echo_server/main.cpp \
            ../../../src/ma/echo/server/error.cpp \
            ../../../src/ma/echo/server/session.cpp \
            ../../../src/ma/echo/server/session_manager.cpp \
            ../../../src/ma/echo/server/qt/custommetatypes.cpp \
            ../../../src/ma/echo/server/qt/service.cpp \
            ../../../src/ma/echo/server/qt/mainform.cpp

FORMS    += ../../../src/ma/echo/server/qt/mainform.ui

win32:INCLUDEPATH += ../../../../boost_1_48_0 
unix:INCLUDEPATH  += /usr/local/include
INCLUDEPATH       += ../../../include

win32:LIBS += -L"./../../../../boost_1_48_0/lib/x86" \
              -lkernel32 -luser32 -lshell32 -luuid -lole32 -ladvapi32 \
              -lws2_32 -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm \
              -lwinspool -lws2_32 -lole32 -luser32 -ladvapi32
unix:LIBS  += -lboost_thread \
              -lboost_system \
              -lboost_date_time

win32:DEFINES += WIN32_LEAN_AND_MEAN _UNICODE UNICODE \
                 WINVER=0x0500 _WIN32_WINNT=0x0500 _WIN32_WINDOWS=0x0410 \
                 _WIN32_IE=0x0600 QT_LARGEFILE_SUPPORT

linux-g++ | linux-g++-64 {
  QMAKE_CXXFLAGS += -std=c++0x -Wstrict-aliasing
}

