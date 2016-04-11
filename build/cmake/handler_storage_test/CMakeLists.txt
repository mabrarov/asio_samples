#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(handler_storage_test)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# Build list of used Boost libraries depending on Boost version
# (use Boost.Chrono if available)
set(BOOST_COMPONENTS "system;thread;date_time;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Google Test framework
if(NOT MA_OWN_GTEST)
    find_package(GTest QUIET)
endif()

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(TEST_SOURCES_DIR "${PROJECT_BASE_DIR}/test")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/detail/binder.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/functional.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/handler_ptr.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/intrusive_list.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/latch.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/service_base.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/thread.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/utility.hpp"
    "${CPP_HEADERS_DIR}/ma/test/io_service_pool.hpp"
    "${CPP_HEADERS_DIR}/ma/bind_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/console_close_signal.hpp"
    "${CPP_HEADERS_DIR}/ma/console_close_signal_service.hpp"
    "${CPP_HEADERS_DIR}/ma/console_close_guard.hpp"
    "${CPP_HEADERS_DIR}/ma/context_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_allocator.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage_service.hpp"
    "${CPP_HEADERS_DIR}/ma/lockable_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/thread_group.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/type_traits.hpp")

set(CPP_SOURCES
    "${SOURCES_DIR}/ma/test/io_service_pool.cpp"
    "${TEST_SOURCES_DIR}/handler_storage_test.cpp")

set(LIBRARIES
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    ma_config)

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

# MSVC 2012 doesn't work with Google Test and tuple
# (even with GTest own tuple implementation)
if (MSVC AND MSVC_VERSION GREATER 1600 AND MSVC_VERSION LESS 1800)
    add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
endif()

add_executable(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
   ${Boost_INCLUDE_DIRS}
   ${GTEST_INCLUDE_DIRS}
   $<TARGET_PROPERTY:ma_config,INTERFACE_INCLUDE_DIRECTORIES>
   ${CPP_HEADERS_DIR})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT MA_NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
    ma_dir_source_group("Source Files" "${TEST_SOURCES_DIR}" "${CPP_SOURCES}")
endif()
