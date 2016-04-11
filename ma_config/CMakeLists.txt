#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(ma_config)

find_package(Boost REQUIRED)

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/config.hpp")

set(CPP_SOURCES
    "${SOURCES_DIR}/fake.cpp")

set(LIBRARIES ${Boost_LIBRARIES})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})

add_library(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME} INTERFACE
    ${Boost_INCLUDE_DIRS}
    ${CPP_HEADERS_DIR})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT MA_NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
