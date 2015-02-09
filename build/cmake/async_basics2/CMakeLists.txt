# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8)
project(async_basics2)

find_package(Boost REQUIRED COMPONENTS system thread date_time regex chrono)

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})

set(SOURCES 
    "${SOURCES_DIR}/ma/tutorial2/async_implementation.cpp"
    "${SOURCES_DIR}/ma/windows/console_signal_service.cpp"
    "${SOURCES_DIR}/ma/console_close_guard.cpp"
    "${SOURCES_DIR}/async_basics2/main.cpp"
)

set(LIBRARIES ${Boost_LIBRARIES})
if(WIN32)
    set(LIBRARIES "${LIBRARIES};ws2_32")
endif()

add_executable(async_basics2 ${SOURCES})
target_link_libraries(async_basics2 ${LIBRARIES})
