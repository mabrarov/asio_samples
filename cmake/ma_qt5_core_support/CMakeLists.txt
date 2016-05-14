#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt5_core_support)

set(cxx_libraries )
set(cxx_compile_options )
set(cxx_compile_definitions )
set(cxx_include_directories )

find_package(Qt5Core QUIET)
if(Qt5Core_FOUND)
    # Fix bug with FindQt and include directories
    if(${Qt5Core_VERSION} VERSION_LESS "5.2")
        list(APPEND cxx_compile_definitions ${Qt5Core_DEFINITIONS})
        list(APPEND cxx_include_directories ${Qt5Core_INCLUDE_DIRS})
        list(APPEND cxx_libraries ma_pic_on)
    endif()

    # Determine linkage type of Qt Core
    get_target_property(qt5_core_type Qt5::Core TYPE)
    if(${qt5_core_type} STREQUAL "STATIC_LIBRARY")

        # Some Qt libraries have no cmake files so wee need to search them ourselves
        # Refer to custom CMake modules:
        #     FindICU.cmake
        #     FindQt5PCRE.cmake

        list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

        find_package(ICU REQUIRED COMPONENTS i18n uc data)
        list(APPEND cxx_libraries ${ICU_LIBRARIES})

        # Starting from Qt 5.5.0 PCRE library is used by QtCore
        if(WIN32 AND NOT(${Qt5Core_VERSION} VERSION_LESS "5.5.0"))
            find_package(Qt5PCRE QUIET)
            if(Qt5PCRE_FOUND)
                list(APPEND cxx_libraries Qt5::PCRE)
            endif()
        endif()

        # Required platform libraries
        if(WIN32)
            list(APPEND cxx_libraries
                "kernel32"
                "user32"
                "shell32"
                "uuid"
                "ole32"
                "advapi32"
                "ws2_32")
        endif()
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
target_compile_options(${PROJECT_NAME} INTERFACE ${cxx_compile_options})
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
