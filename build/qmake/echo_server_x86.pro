# -------------------------------------------------
# Project created by QtCreator 2009-09-12T12:08:08
# -------------------------------------------------
QT       -= core gui
TARGET   =  echo_server
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE =  app

SOURCES  += ../../src/console_controller.cpp \
            ../../src/echo_server/main.cpp

HEADERS  += ../../include/ma/handler_storage_service.hpp \
            ../../include/ma/handler_storage.hpp \
            ../../include/ma/handler_invoke_helpers.hpp \
            ../../include/ma/handler_allocation.hpp \
            ../../include/ma/handler_alloc_helpers.hpp \
            ../../include/ma/cyclic_buffer.hpp \
            ../../include/console_controller.hpp \
            ../../include/ma/echo/server/session.hpp \
            ../../include/ma/echo/server/session_manager.hpp

LIBS        += -L../../../boost_1_40_0/stage32/lib
INCLUDEPATH += ../../../boost_1_40_0 ../../include
DEFINES     += WIN32_LEAN_AND_MEAN BOOST_HAS_THREADS
