#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_performance_test_client)

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

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

set(HEADERS
    "${HEADERS_DIR}/ma/detail/binder.hpp"
    "${HEADERS_DIR}/ma/detail/functional.hpp"
    "${HEADERS_DIR}/ma/detail/memory.hpp"
    "${HEADERS_DIR}/ma/detail/thread.hpp"
    "${HEADERS_DIR}/ma/async_connect.hpp"
    "${HEADERS_DIR}/ma/bind_handler.hpp"
    "${HEADERS_DIR}/ma/config.hpp"
    "${HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${HEADERS_DIR}/ma/cyclic_buffer.hpp"
    "${HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_allocator.hpp"
    "${HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${HEADERS_DIR}/ma/limited_int.hpp"
    "${HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${HEADERS_DIR}/ma/strand.hpp"
    "${HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${HEADERS_DIR}/ma/thread_group.hpp"
    "${HEADERS_DIR}/ma/type_traits.hpp")

set(SOURCES
    "${SOURCES_DIR}/asio_performance_test_client/main.cpp"
)

set(LIBRARIES ${Boost_LIBRARIES})

# Required Winsock libraries
if(WIN32)
    set(LIBRARIES "ws2_32;mswsock;${LIBRARIES}")
endif()

# Boost chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    set(LIBRARIES "${LIBRARIES};rt")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})
add_executable(asio_performance_test_client ${HEADERS} ${SOURCES})
target_link_libraries(asio_performance_test_client ${LIBRARIES})
