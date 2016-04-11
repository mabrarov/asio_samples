#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(handler_storage_test)

set(project_base_dir     "${PROJECT_SOURCE_DIR}/../../..")
set(cxx_headers_dir      "${project_base_dir}/include")
set(cxx_sources_dir      "${project_base_dir}/src")
set(cxx_test_sources_dir "${project_base_dir}/test")

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Build list of used Boost libraries depending on Boost version
# (use Boost.Chrono if available)
set(BOOST_COMPONENTS "system;thread;date_time;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(ma_boost_has_chrono TRUE)
else()
    set(ma_boost_has_chrono FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Google Test framework
if(NOT ma_own_gtest)
    find_package(GTest QUIET)
endif()

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()

# MSVC 2012 doesn't work with Google Test and tuple
# (even with GTest own tuple implementation)
if (MSVC AND MSVC_VERSION GREATER 1600 AND MSVC_VERSION LESS 1800)
    list(APPEND cxx_compile_definitions GTEST_HAS_TR1_TUPLE=0)
endif()

list(APPEND cxx_headers
    "${cxx_headers_dir}/ma/detail/binder.hpp"
    "${cxx_headers_dir}/ma/detail/handler_ptr.hpp"
    "${cxx_headers_dir}/ma/detail/intrusive_list.hpp"
    "${cxx_headers_dir}/ma/detail/service_base.hpp"
    "${cxx_headers_dir}/ma/test/io_service_pool.hpp"
    "${cxx_headers_dir}/ma/bind_handler.hpp"
    "${cxx_headers_dir}/ma/console_close_signal.hpp"
    "${cxx_headers_dir}/ma/console_close_signal_service.hpp"
    "${cxx_headers_dir}/ma/console_close_guard.hpp"
    "${cxx_headers_dir}/ma/context_alloc_handler.hpp"
    "${cxx_headers_dir}/ma/context_wrapped_handler.hpp"
    "${cxx_headers_dir}/ma/custom_alloc_handler.hpp"
    "${cxx_headers_dir}/ma/handler_alloc_helpers.hpp"
    "${cxx_headers_dir}/ma/handler_allocator.hpp"
    "${cxx_headers_dir}/ma/handler_cont_helpers.hpp"
    "${cxx_headers_dir}/ma/handler_invoke_helpers.hpp"
    "${cxx_headers_dir}/ma/handler_storage.hpp"
    "${cxx_headers_dir}/ma/handler_storage_service.hpp"
    "${cxx_headers_dir}/ma/lockable_wrapped_handler.hpp"
    "${cxx_headers_dir}/ma/strand_wrapped_handler.hpp"
    "${cxx_headers_dir}/ma/thread_group.hpp")

list(APPEND cxx_sources
    "${cxx_sources_dir}/ma/test/io_service_pool.cpp"
    "${cxx_test_sources_dir}/handler_storage_test.cpp")

list(APPEND cxx_libraries
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    ma_config
    ma_compat)

# Required Winsock libraries
if(WIN32)
    list(APPEND cxx_libraries
        "ws2_32"
        "mswsock")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND ma_boost_has_chrono)
    list(APPEND cxx_libraries
        "rt")
endif()

add_executable(${PROJECT_NAME} ${cxx_headers} ${cxx_sources})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} PRIVATE
   ${Boost_INCLUDE_DIRS}
   ${GTEST_INCLUDE_DIRS}
   $<TARGET_PROPERTY:ma_config,INTERFACE_INCLUDE_DIRECTORIES>
   $<TARGET_PROPERTY:ma_compat,INTERFACE_INCLUDE_DIRECTORIES>
   ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    ${cxx_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
    ma_dir_source_group("Source Files" "${cxx_test_sources_dir}" "${cxx_sources}")
endif()
