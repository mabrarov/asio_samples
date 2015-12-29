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
# (use Boost.Chrono if available)
set(BOOST_COMPONENTS "system;thread;date_time;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Determine major version of Qt to use (Qt 5 is predeferred)
if(NOT DEFINED QT_ECHO_SERVER_USE_QT5)
    set(QT_ECHO_SERVER_USE_QT5 TRUE)
    find_package(Qt5Widgets QUIET)
    if(NOT Qt5Widgets_FOUND)
        set(QT_ECHO_SERVER_USE_QT5 FALSE)
    endif()
endif()

# Find required Qt libraries
if(QT_ECHO_SERVER_USE_QT5)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Gui     REQUIRED)
    find_package(Qt5Core    REQUIRED)
    set(Qt_LIBRARIES "Qt5::Core;Qt5::Gui;Qt5::Widgets")
else()
    find_package(Qt4 REQUIRED QtCore QtGui)
    set(Qt_LIBRARIES "Qt4::QtCore;Qt4::QtGui")
endif()

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(UIC_FORMS
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.ui")

set(QRC_RESOURCES )

set(MOC_HEADERS
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/mainform.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/service.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/serviceforwardsignal.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/serviceservantsignal.h")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/custommetatypes.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/execution_config.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/execution_config_fwd.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/serviceforwardsignal_fwd.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/serviceservantsignal_fwd.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/servicestate.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/service_fwd.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/meta_type_register_error.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/qt/signal_connect_error.h"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_config_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_config_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_stats.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_stats_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_config.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_config.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/error.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/pooled_session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_factory_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/simple_session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/binder.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/functional.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/handler_ptr.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/intrusive_list.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/service_base.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/thread.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/tuple.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/utility.hpp"
    "${CPP_HEADERS_DIR}/ma/bind_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/config.hpp"
    "${CPP_HEADERS_DIR}/ma/context_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/cyclic_buffer.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_allocator.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage_service.hpp"
    "${CPP_HEADERS_DIR}/ma/limited_int.hpp"
    "${CPP_HEADERS_DIR}/ma/shared_ptr_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/sp_intrusive_list.hpp"
    "${CPP_HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${CPP_HEADERS_DIR}/ma/strand.hpp"
    "${CPP_HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/thread_group.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/type_traits.hpp"
    ${MOC_HEADERS})

set(CPP_SOURCES
    "${SOURCES_DIR}/ma/echo/server/qt/custommetatypes.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/service.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.cpp"
    "${SOURCES_DIR}/ma/echo/server/error.cpp"
    "${SOURCES_DIR}/ma/echo/server/session.cpp"
    "${SOURCES_DIR}/ma/echo/server/session_manager.cpp"
    "${SOURCES_DIR}/ma/echo/server/pooled_session_factory.cpp"
    "${SOURCES_DIR}/ma/echo/server/simple_session_factory.cpp"
    "${SOURCES_DIR}/qt_echo_server/main.cpp")

set(NATIVE_RESOURCES )
set(NATIVE_RESOURCE_HEADERS )
if(WIN32)
    list(APPEND NATIVE_RESOURCES "${SOURCES_DIR}/qt_echo_server/qt_echo_server.rc")
endif()

set(LIBRARIES
    "${Boost_LIBRARIES}"
    "${Qt_LIBRARIES}")

if(QT_ECHO_SERVER_USE_QT5)
    # Determine linkage type of Qt libraries (via linkage type of Qt Core library)
    get_property(Qt5_Core_TYPE TARGET Qt5::Core PROPERTY TYPE)
    set(QT5_IS_STATIC false)
    if(${Qt5_Core_TYPE} STREQUAL "STATIC_LIBRARY")
        set(QT5_IS_STATIC true)
    endif()

    # Statically linked Qt requires:
    #     All libraries used by Qt internally to be linked explicitly to the target.
    #     Used Qt plugins to be linked explicitly to the target.
    if(QT5_IS_STATIC)
        # Qt CMake files use IMPORTED_LINK_DEPENDENT_LIBRARIES instead of INTERFACE_LINK_LIBRARIES for OpenGL dependencies.
        # So we need to add those transitive (in case of static linkage) dependencies explicitly.
        list(APPEND LIBRARIES
            "${Qt5Gui_EGL_LIBRARIES}"
            "${Qt5Gui_OPENGL_LIBRARIES}")

        # Add required platform integration plugins
        if(WIN32)
            list(APPEND LIBRARIES "Qt5::QWindowsIntegrationPlugin")
        else()
            # todo:
            # add support for the rest of platforms supported by Qt
            # by linking with platform specific plugin
            # (add platfrom integration plugin to LIBRARIES)
        endif()

        # Some Qt libraries have no cmake files so wee need to search them ourselves
        # Refer to custom CMake modules:
        #     FindICU.cmake
        #     FindQt5PlatformSupport.cmake
        #     FindQt5HarfBuzzNG.cmake
        #     FindQt5FreeType.cmake
        #     FindQt5PCRE.cmake

        find_package(Qt5PlatformSupport REQUIRED)
        list(APPEND LIBRARIES "Qt5::PlatformSupport")

        find_package(ICU REQUIRED COMPONENTS i18n uc data)
        list(APPEND LIBRARIES "${ICU_LIBRARIES}")

        # Starting from Qt 5.4.0 HarfBuzz-NG library is included (at least Windows version of Qt 5.4.0)
        if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.4.0"))
            find_package(Qt5HarfBuzzNG QUIET)
            if(Qt5HarfBuzzNG_FOUND)
                list(APPEND LIBRARIES "Qt5::HarfBuzzNG")
            endif()
        endif()

        # Starting from Qt 5.5.0 FreeType library is used by QtGui
        if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.5.0"))
            find_package(Qt5FreeType QUIET)
            if(Qt5FreeType_FOUND)
                list(APPEND LIBRARIES "Qt5::FreeType")
            endif()
        endif()

        # Starting from Qt 5.5.0 PCRE library is used by QtCore
        if(WIN32 AND NOT(${Qt5Core_VERSION} VERSION_LESS "5.5.0"))
            find_package(Qt5PCRE QUIET)
            if(Qt5PCRE_FOUND)
                list(APPEND LIBRARIES "Qt5::PCRE")
            endif()
        endif()
    endif()
else()
    # Statically linked Qt requires:
    #     All libraries used by Qt internally to be linked explicitly to the target.
    #     Used Qt plugins to be linked explicitly to the target.
    if(QT_IS_STATIC)
        # Nothing to add as no Qt 4 plugins are used.
    endif()
endif()

# Required Windows libraries
if(WIN32)
    list(APPEND LIBRARIES
        "ws2_32"
        "mswsock"
        "comdlg32"
        "uuid"
        "ole32"
        "oleaut32"
        "advapi32"
        "imm32"
        "winmm"
        "winspool")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    list(APPEND LIBRARIES
        "rt")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})

# Required for Qt UIC
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Wrap Qt generated sources.
if(QT_ECHO_SERVER_USE_QT5)
    qt5_wrap_cpp(MOC_GENERATED_SOURCES ${MOC_HEADERS})
    qt5_wrap_ui(UIC_GENERATED_HEADERS ${UIC_FORMS})
    qt5_add_resources(QRC_GENERATED_SOURCES ${QRC_RESOURCES})
else()
    qt4_wrap_cpp(MOC_GENERATED_SOURCES ${MOC_HEADERS})
    qt4_wrap_ui(UIC_GENERATED_HEADERS ${UIC_FORMS})
    qt4_add_resources(QRC_GENERATED_SOURCES ${QRC_RESOURCES})
endif()

add_executable(${PROJECT_NAME} WIN32
    ${CPP_HEADERS} ${CPP_SOURCES}
    ${NATIVE_RESOURCES} ${NATIVE_RESOURCE_HEADERS}
    ${MOC_GENERATED_SOURCES} ${QRC_GENERATED_SOURCES} ${UIC_GENERATED_HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

source_group("Form Files" FILES
    ${UIC_FORMS})
source_group("Generated Files" FILES
    ${MOC_GENERATED_SOURCES}
    ${QRC_GENERATED_SOURCES}
    ${UIC_GENERATED_HEADERS})
source_group("Resource Files" FILES
    ${NATIVE_RESOURCES}
    ${NATIVE_RESOURCE_HEADERS}
    ${QRC_RESOURCES})

if(NOT NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
