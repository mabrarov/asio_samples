#
# Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       -= core gui
TARGET    = nmea_client
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
            ../../../include/ma/codecvt_cast.hpp \
            ../../../include/ma/console_controller.hpp \
            ../../../include/ma/nmea/frame.hpp \
            ../../../include/ma/nmea/error.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/type_traits.hpp \
            ../../../include/ma/shared_ptr_factory.hpp \
            ../../../include/ma/nmea/cyclic_read_session_fwd.hpp \
            ../../../include/ma/nmea/cyclic_read_session.hpp

SOURCES  += ../../../src/ma/console_controller.cpp \
            ../../../src/nmea_client/main.cpp \
            ../../../src/ma/nmea/error.cpp \
            ../../../src/ma/nmea/cyclic_read_session.cpp

INCLUDEPATH += $${BOOST_INCLUDE} \
               ../../../include

LIBS       += -L$${BOOST_LIB}
unix:LIBS  += $${BOOST_LIB}/libboost_system.a \
              $${BOOST_LIB}/libboost_thread.a \
              $${BOOST_LIB}/libboost_date_time.a

win32:DEFINES += WINVER=0x0500 \
                 _WIN32_WINNT=0x0500
