QT       -= core gui
TARGET   =  nmea_client
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE =  app

SOURCES  += ../../../src/ma/console_controller.cpp \
            ../../../src/nmea_client/main.cpp

HEADERS  += ../../../include/ma/handler_storage_service.hpp \
            ../../../include/ma/handler_storage.hpp \
            ../../../include/ma/handler_invoke_helpers.hpp \
            ../../../include/ma/bind_asio_handler.hpp \
            ../../../include/ma/handler_allocation.hpp \
            ../../../include/ma/handler_alloc_helpers.hpp \
            ../../../include/ma/codecvt_cast.hpp \
            ../../../include/ma/console_controller.hpp \
            ../../../include/ma/nmea/cyclic_read_session.hpp

LIBS        += -L../../../../boost_1_43_0/lib/x86
INCLUDEPATH += ../../../../boost_1_43_0 \
               ../../../include
DEFINES     += WIN32_LEAN_AND_MEAN BOOST_HAS_THREADS _UNICODE UNICODE
