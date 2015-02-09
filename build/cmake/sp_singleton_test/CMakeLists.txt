# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8)
project(sp_singleton_test)

find_package(Boost REQUIRED COMPONENTS system thread date_time chrono)

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})

set(SOURCES 
    "${SOURCES_DIR}/sp_singleton_test/main.cpp"
)

add_executable(sp_singleton_test ${SOURCES})
target_link_libraries(sp_singleton_test ${Boost_LIBRARIES})
