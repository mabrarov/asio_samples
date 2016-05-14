#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt4_gui_support)

set(cxx_libraries )

list(APPEND cxx_libraries ma_qt4_core_support)

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

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
