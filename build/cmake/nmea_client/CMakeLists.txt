# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8)
project(nmea_client)

find_package(Boost REQUIRED)

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})

set(SOURCES 
    "${SOURCES_DIR}/ma/nmea/error.cpp"
    "${SOURCES_DIR}/ma/nmea/cyclic_read_session.cpp"
    "${SOURCES_DIR}/ma/windows/console_signal_service.cpp"
    "${SOURCES_DIR}/ma/console_close_guard.cpp"
    "${SOURCES_DIR}/nmea_client/main.cpp"
)

add_executable(nmea_client ${SOURCES})
