#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(ma_async_connect)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()

list(APPEND cxx_headers
    "${cxx_headers_dir}/ma/async_connect.hpp")

list(APPEND cxx_sources
    "${cxx_sources_dir}/fake.cpp")

list(APPEND cxx_libraries
    ${Boost_LIBRARIES}
    ma_config
    ma_compat
    ma_bind_handler
    ma_helpers)

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
    $<TARGET_PROPERTY:ma_config,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ma_compat,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ma_bind_handler,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:ma_helpers,INTERFACE_INCLUDE_DIRECTORIES>
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    ${cxx_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()
