#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt5_core_static)

set(cxx_libraries )

find_package(Qt5Core QUIET)
if(Qt5Core_FOUND)
    # Determine linkage type of Qt Core
    get_target_property(qt5_core_type Qt5::Core TYPE)
    if(${qt5_core_type} STREQUAL "STATIC_LIBRARY")

        # Some Qt libraries have no cmake files so wee need to search them ourselves
        # Refer to custom CMake modules:
        #     FindICU.cmake
        #     FindQt5PCRE.cmake

        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}")

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
                "ws2_32"
                "mswsock"
                "uuid"
                "ole32"
                "oleaut32"
                "advapi32")
        endif()
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
