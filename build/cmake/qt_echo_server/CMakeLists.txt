# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(qt_echo_server)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Build list of used Boost libraries depending on Boost version 
# (use chono if available)
set(BOOST_COMPONENTS "system;thread;date_time;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Find required Qt libraries
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui     REQUIRED)
find_package(Qt5Core    REQUIRED)

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

set(MOC_HEADERS
    "${HEADERS_DIR}/ma/echo/server/qt/mainform.h"
    "${HEADERS_DIR}/ma/echo/server/qt/service.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceforwardsignal.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceservantsignal.h")

set(UIC_FORMS
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.ui")

set(RCC_RESOURCES
    )

set(HEADERS
    "${HEADERS_DIR}/ma/echo/server/qt/custommetatypes.h"
    "${HEADERS_DIR}/ma/echo/server/qt/execution_config.h"
    "${HEADERS_DIR}/ma/echo/server/qt/execution_config_fwd.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceforwardsignal_fwd.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceservantsignal_fwd.h"
    "${HEADERS_DIR}/ma/echo/server/qt/servicestate.h"
    "${HEADERS_DIR}/ma/echo/server/qt/service_fwd.h"
    "${HEADERS_DIR}/ma/echo/server/qt/meta_type_register_error.h"
    "${HEADERS_DIR}/ma/echo/server/qt/signal_connect_error.h"
    "${HEADERS_DIR}/ma/echo/server/session_config_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_config_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_stats.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_stats_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_config.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_config.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/error.hpp"
    "${HEADERS_DIR}/ma/echo/server/session.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/pooled_session_factory.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_factory.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_factory_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/simple_session_factory.hpp"
    "${HEADERS_DIR}/ma/detail/binder.hpp"
    "${HEADERS_DIR}/ma/detail/functional.hpp"
    "${HEADERS_DIR}/ma/detail/handler_ptr.hpp"
    "${HEADERS_DIR}/ma/detail/intrusive_list.hpp"
    "${HEADERS_DIR}/ma/detail/memory.hpp"
    "${HEADERS_DIR}/ma/detail/service_base.hpp"
    "${HEADERS_DIR}/ma/detail/thread.hpp"
    "${HEADERS_DIR}/ma/detail/tuple.hpp"
    "${HEADERS_DIR}/ma/bind_handler.hpp"
    "${HEADERS_DIR}/ma/config.hpp"
    "${HEADERS_DIR}/ma/context_alloc_handler.hpp"
    "${HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${HEADERS_DIR}/ma/cyclic_buffer.hpp"
    "${HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_allocator.hpp"
    "${HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_storage.hpp"
    "${HEADERS_DIR}/ma/handler_storage_service.hpp"
    "${HEADERS_DIR}/ma/limited_int.hpp"
    "${HEADERS_DIR}/ma/shared_ptr_factory.hpp"
    "${HEADERS_DIR}/ma/sp_intrusive_list.hpp"
    "${HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${HEADERS_DIR}/ma/strand.hpp"
    "${HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${HEADERS_DIR}/ma/thread_group.hpp"
    "${HEADERS_DIR}/ma/type_traits.hpp"
    ${MOC_HEADERS})

set(SOURCES
    "${SOURCES_DIR}/ma/echo/server/qt/custommetatypes.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/service.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.cpp"
    "${SOURCES_DIR}/ma/echo/server/error.cpp"
    "${SOURCES_DIR}/ma/echo/server/session.cpp"
    "${SOURCES_DIR}/ma/echo/server/session_manager.cpp"
    "${SOURCES_DIR}/ma/echo/server/pooled_session_factory.cpp"
    "${SOURCES_DIR}/ma/echo/server/simple_session_factory.cpp"
    "${SOURCES_DIR}/qt_echo_server/main.cpp"
    "${SOURCES_DIR}/qt_echo_server/qt_echo_server.rc"
    ${UIC_FORMS})

set(LIBRARIES "${Boost_LIBRARIES};Qt5::Core;Qt5::Gui;Qt5::Widgets")

# Required Winsock libraries
if(WIN32)
    set(LIBRARIES "ws2_32;mswsock;comdlg32;uuid;ole32;oleaut32;advapi32;imm32;winmm;winspool;${LIBRARIES}")    
endif()

# Determine linkage type of Qt libraries (via linkage type of Qt Core library)
get_property(Qt5_Core_TYPE TARGET Qt5::Core PROPERTY TYPE)
set(QT5_STATIC_LINKAGE false)
if(${Qt5_Core_TYPE} STREQUAL "STATIC_LIBRARY")
    set(QT5_STATIC_LINKAGE true)
endif()

# Statically linked Qt requires:
#     All libraries used by Qt internally to be linked explicitly to the target.
#     Used Qt plugins to be linked explicitly to the target.
if(QT5_STATIC_LINKAGE)
    # Qt CMake files use IMPORTED_LINK_DEPENDENT_LIBRARIES instead of INTERFACE_LINK_LIBRARIES for OpenGL dependencies.
    # So we need to add those transitive (in case of static linkage) dependencies explicitly.
    set(LIBRARIES "${LIBRARIES};${Qt5Gui_EGL_LIBRARIES};${Qt5Gui_OPENGL_LIBRARIES}")

    # Add (all) Qt plugins for Qt Core, Qt GUI and Qt Widgets libraries
    set(LIBRARIES "${LIBRARIES};${Qt5Core_PLUGINS};${Qt5Gui_PLUGINS};${Qt5Widgets_PLUGINS}")

    # Some Qt libraries have no cmake files so wee need to search them ourselves
    # Refer to custom CMake modules:
    #     FindICU.cmake
    #     FindQt5PlatformSupport.cmake
    #     FindQt5HarfBuzzNG.cmake

    find_package(ICU REQUIRED COMPONENTS data uc i18n)
    set(LIBRARIES "${LIBRARIES};${ICU_LIBRARIES}")

    find_package(Qt5PlatformSupport REQUIRED)
    set(LIBRARIES "${LIBRARIES};Qt5::PlatformSupport")

    # Starting from Qt 5.4.0 HarfBuzz-NG library is included (at least Windows version of Qt 5.4.0)
    if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.4.0"))
        find_package(Qt5HarfBuzzNG)
        if(Qt5HarfBuzzNG_FOUND)
            set(LIBRARIES "${LIBRARIES};Qt5::HarfBuzzNG")
        endif()
    endif()
endif()

# Boost chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    set(LIBRARIES "${LIBRARIES};rt")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Wrap Qt generated sources.
qt5_wrap_cpp(MOC_GENERATED_SOURCES ${MOC_HEADERS})
qt5_wrap_ui(UIC_GENERATED_HEADERS ${UIC_FORMS})
qt5_add_resources(RCC_GENERATED_SOURCES ${RCC_RESOURCES})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})
add_executable(qt_echo_server WIN32 
    ${HEADERS} ${SOURCES} 
    ${MOC_GENERATED_SOURCES} ${RCC_GENERATED_SOURCES} ${UIC_GENERATED_HEADERS})
target_link_libraries(qt_echo_server ${LIBRARIES})
