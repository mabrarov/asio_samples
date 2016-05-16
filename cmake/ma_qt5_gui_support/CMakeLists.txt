#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt5_gui_support)

set(cxx_libraries )
set(cxx_compile_options )
set(cxx_compile_definitions )
set(cxx_include_directories )

find_package(Qt5Gui QUIET)
if(Qt5Gui_FOUND)
    # Fix bug with FindQt and include directories
    if(${Qt5Gui_VERSION} VERSION_LESS "5.2")
        list(APPEND cxx_compile_definitions ${Qt5Gui_DEFINITIONS})
        list(APPEND cxx_include_directories ${Qt5Gui_INCLUDE_DIRS})
        list(APPEND cxx_libraries ma_pic_on)
    endif()

    # Determine linkage type of Qt Gui
    get_target_property(qt5_gui_type Qt5::Gui TYPE)
    if(${qt5_gui_type} STREQUAL "STATIC_LIBRARY")
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

        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

        find_package(Qt5PlatformSupport REQUIRED)
        list(APPEND cxx_libraries Qt5::PlatformSupport)

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

        list(APPEND cxx_libraries ma_qt5_core_support)

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
                "gdi32"
                "comdlg32"
                "oleaut32"
                "imm32"
                "winmm"
                "ws2_32"
                "ole32"
                "user32"
                "advapi32")
        endif()
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
target_compile_options(${PROJECT_NAME} INTERFACE ${cxx_compile_options})
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
