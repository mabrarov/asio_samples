# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(async_connect)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Build list of used Boost libraries depending on Boost version 
# (use chono and timer if available)
set(BOOST_COMPONENTS "system;thread;date_time;program_options;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()
if(NOT(${Boost_VERSION} LESS 104800))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};timer")
    set(BOOST_HAS_TIMER TRUE)
else()
    set(BOOST_HAS_TIMER FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/detail/binder.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/functional.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/thread.hpp"
    "${CPP_HEADERS_DIR}/ma/async_connect.hpp"
    "${CPP_HEADERS_DIR}/ma/bind_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/config.hpp"
    "${CPP_HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_allocator.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/limited_int.hpp"
    "${CPP_HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${CPP_HEADERS_DIR}/ma/strand.hpp"
    "${CPP_HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/thread_group.hpp"
    "${CPP_HEADERS_DIR}/ma/type_traits.hpp")

set(CPP_SOURCES
    "${SOURCES_DIR}/async_connect/main.cpp")

set(LIBRARIES ${Boost_LIBRARIES})

# Required Winsock libraries
if(WIN32)
    list(APPEND LIBRARIES 
        "ws2_32"
        "mswsock")
endif()

# Boost chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    list(APPEND LIBRARIES 
        "rt")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})
add_executable(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT NO_CMAKE_DIR_SOURCE_GROUP)
    # Group header files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
