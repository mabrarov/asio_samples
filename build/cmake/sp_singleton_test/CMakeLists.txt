#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(sp_singleton_test)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# Build list of used Boost libraries depending on Boost version
# (use Boost.Chrono if available)
set(BOOST_COMPONENTS "system;thread;date_time")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Google Test libraries
find_package(GTest QUIET)

# Use local copy of Google Test if it wasn't provided
if(NOT GTEST_FOUND)
    set(GTEST_LIBRARIES      gtest)
    set(GTEST_MAIN_LIBRARIES gtest_main)
    set(GTEST_BOTH_LIBRARIES gtest gtest_main)
    get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
    # Workaround for old version of Google Test
    if(NOT GTEST_INCLUDE_DIRS)
        set(GTEST_INCLUDE_DIRS ${gtest_SOURCE_DIR}/include)
    endif()
endif()

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(TEST_SOURCES_DIR "${PROJECT_BASE_DIR}/test")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/detail/functional.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/latch.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/sp_singleton.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/thread.hpp"
    "${CPP_HEADERS_DIR}/ma/config.hpp"
    "${CPP_HEADERS_DIR}/ma/limited_int.hpp"
    "${CPP_HEADERS_DIR}/ma/shared_ptr_factory.hpp")

set(CPP_SOURCES
    "${TEST_SOURCES_DIR}/sp_singleton_test.cpp")

set(LIBRARIES
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES})

# Required Winsock libraries
if(WIN32)
    list(APPEND LIBRARIES
        "ws2_32"
        "mswsock")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    list(APPEND LIBRARIES
        "rt")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})
add_executable(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
