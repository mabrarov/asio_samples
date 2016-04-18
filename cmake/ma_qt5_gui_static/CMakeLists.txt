#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt5_gui_static)

# Setup path for additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}")

set(cxx_libraries )

find_package(Qt5Gui QUIET)
if(Qt5Gui_FOUND)
    # Add required platform integration plugins
    if(WIN32)
        list(APPEND cxx_libraries Qt5::QWindowsIntegrationPlugin)
    else()
        # todo:
        # add support for the rest of platforms supported by Qt
        # by linking with platform specific plugin
        # (add platfrom integration plugin to cxx_libraries)
    endif()

    # Some Qt libraries have no cmake files so wee need to search them ourselves
    # Refer to custom CMake modules:
    #     FindQt5PlatformSupport.cmake
    #     FindQt5HarfBuzzNG.cmake
    #     FindQt5FreeType.cmake

    find_package(Qt5PlatformSupport REQUIRED)
    list(APPEND cxx_libraries Qt5::PlatformSupport)

    # PlatformSupport and platform integration plugins use Qt Core and Qt Gui
    list(APPEND cxx_libraries
        Qt5::Core
        Qt5::Gui)

    # Starting from Qt 5.4.0 HarfBuzz-NG library is included (at least Windows
    # version of Qt 5.4.0)
    if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.4.0"))
        find_package(Qt5HarfBuzzNG QUIET)
        if(Qt5HarfBuzzNG_FOUND)
            list(APPEND cxx_libraries Qt5::HarfBuzzNG)
        endif()
    endif()

    # Starting from Qt 5.5.0 FreeType library is used by QtGui
    if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.5.0"))
        find_package(Qt5FreeType QUIET)
        if(Qt5FreeType_FOUND)
            list(APPEND cxx_libraries Qt5::FreeType)
        endif()
    endif()

    list(APPEND cxx_libraries ma_qt5_core_static)

    # Qt CMake files use IMPORTED_LINK_DEPENDENT_LIBRARIES instead of
    # INTERFACE_LINK_LIBRARIES for OpenGL dependencies.
    # So we need to add those transitive (in case of static linkage)
    # dependencies explicitly.
    list(APPEND cxx_libraries
        ${Qt5Gui_EGL_LIBRARIES}
        ${Qt5Gui_OPENGL_LIBRARIES})

    # Required platform libraries
    if(WIN32)
        list(APPEND cxx_libraries
            "comdlg32"
            "imm32"
            "winmm"
            "winspool")
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
