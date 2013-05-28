#
# Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

TEMPLATE  = app
QT       -= core gui
TARGET    = handler_storage_test
CONFIG   += console thread
CONFIG   -= app_bundle

# Common project configuration
include(../config.pri)

HEADERS  += ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/config.hpp \
            ../../../include/ma/context_alloc_handler.hpp \
            ../../../include/ma/context_invoke_handler.hpp \
            ../../../include/ma/context_wrapped_handler.hpp \
            ../../../include/ma/custom_alloc_handler.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/handler_allocator.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \            
            ../../../include/ma/handler_storage.hpp \
            ../../../include/ma/handler_storage_service.hpp \
            ../../../include/ma/lockable_wrapped_handler.hpp \
            ../../../include/ma/strand_wrapped_handler.hpp \
            ../../../include/ma/type_traits.hpp

SOURCES  += ../../../src/handler_storage_test/main.cpp

INCLUDEPATH += $${BOOST_INCLUDE} \
               ../../../include

LIBS       += -L$${BOOST_LIB}
unix:LIBS  += $${BOOST_LIB}/libboost_system.a \
              $${BOOST_LIB}/libboost_thread.a \
              $${BOOST_LIB}/libboost_date_time.a

win32:DEFINES += WINVER=0x0500 \
                 _WIN32_WINNT=0x0500
