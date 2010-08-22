QT       -= core gui
TARGET   =  echo_server3
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE =  app

SOURCES  += ../../../src/ma/console_controller.cpp \
            ../../../src/echo_server3/main.cpp \
            ../../../src/ma/echo/server3/session.cpp \
            ../../../src/ma/echo/server3/session_manager.cpp \
            ../../../src/ma/echo/server3/session_proxy.cpp \
            ../../../src/ma/echo/server3/session_proxy_list.cpp \
            ../../../src/ma/echo/server3/simple_allocator.cpp


HEADERS  += ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/handler_allocation.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/cyclic_buffer.hpp \
            ../../../include/ma/console_controller.hpp \
            ../../../include/ma/echo/server3/allocator.h \
            ../../../include/ma/echo/server3/allocator_fwd.h \
            ../../../include/ma/echo/server3/session.h \
            ../../../include/ma/echo/server3/session_fwd.h \
            ../../../include/ma/echo/server3/session_handler.h \
            ../../../include/ma/echo/server3/session_handler_fwd.h \
            ../../../include/ma/echo/server3/session_manager.h \
            ../../../include/ma/echo/server3/session_manager_fwd.h \
            ../../../include/ma/echo/server3/session_manager_handler.h \
            ../../../include/ma/echo/server3/session_manager_handler_fwd.h \
            ../../../include/ma/echo/server3/session_proxy.h \
            ../../../include/ma/echo/server3/session_proxy_fwd.h \
            ../../../include/ma/echo/server3/session_proxy_list.h \
            ../../../include/ma/echo/server3/simple_allocator.h

LIBS        += -L../../../../boost_1_44_0/lib/x86
INCLUDEPATH += ../../../../boost_1_44_0 \
               ../../../include
DEFINES     += WIN32_LEAN_AND_MEAN BOOST_HAS_THREADS _UNICODE UNICODE
