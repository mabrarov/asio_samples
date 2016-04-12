#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(shared_ptr_factory_test)

find_package(Boost REQUIRED)

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# Google Test framework
if(NOT ASIO_SAMPLES_OWN_GTEST)
    find_package(GTest QUIET)
endif()

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(TEST_SOURCES_DIR "${PROJECT_BASE_DIR}/test")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/config.hpp"
    "${CPP_HEADERS_DIR}/ma/shared_ptr_factory.hpp")

set(CPP_SOURCES
    "${TEST_SOURCES_DIR}/shared_ptr_factory_test.cpp")

set(LIBRARIES
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES})

# MSVC 2012 doesn't work with Google Test and tuple
# (even with GTest own tuple implementation)
if (MSVC AND MSVC_VERSION GREATER 1600 AND MSVC_VERSION LESS 1800)
    add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})
add_executable(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT ASIO_SAMPLES_NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
