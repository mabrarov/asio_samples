#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(shared_ptr_factory_test)

set(project_base_dir     "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir      "${project_base_dir}/include")
set(cxx_sources_dir      "${project_base_dir}/src")
set(cxx_test_sources_dir "${project_base_dir}/test")

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )

find_package(Boost REQUIRED)

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
    "${cxx_headers_dir}/ma/shared_ptr_factory.hpp")

list(APPEND cxx_sources
    "${cxx_test_sources_dir}/shared_ptr_factory_test.cpp")

list(APPEND cxx_libraries
    ${Boost_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    ma_config
    ma_compat)

add_executable(${PROJECT_NAME}
    ${cxx_headers}
    ${cxx_sources})
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
endif()
