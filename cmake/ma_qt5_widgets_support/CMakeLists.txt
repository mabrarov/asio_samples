#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt5_widgets_support)

set(cxx_libraries )
set(cxx_compile_options )
set(cxx_compile_definitions )
set(cxx_include_directories )

find_package(Qt5Widgets QUIET)
if(Qt5Widgets_FOUND)
    # Fix bug with FindQt and include directories
    if(${Qt5Widgets_VERSION} VERSION_LESS "5.2")
        list(APPEND cxx_compile_definitions ${Qt5Widgets_DEFINITIONS})
        list(APPEND cxx_include_directories ${Qt5Widgets_INCLUDE_DIRS})
        list(APPEND cxx_libraries ma_pic_on)
    endif()

    list(APPEND cxx_libraries
        ma_qt5_core_support
        ma_qt5_gui_support)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
target_compile_options(${PROJECT_NAME} INTERFACE ${cxx_compile_options})
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
