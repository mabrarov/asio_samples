#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(ma_compat)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )
set(boost_components )

# Select used Boost libraries
list(APPEND boost_components
    thread)

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${boost_components})

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()

list(APPEND cxx_headers
    "${cxx_headers_dir}/ma/detail/functional.hpp"
    "${cxx_headers_dir}/ma/detail/latch.hpp"
    "${cxx_headers_dir}/ma/detail/memory.hpp"
    "${cxx_headers_dir}/ma/detail/thread.hpp"
    "${cxx_headers_dir}/ma/detail/tuple.hpp"
    "${cxx_headers_dir}/ma/detail/type_traits.hpp"
    "${cxx_headers_dir}/ma/detail/utility.hpp")

list(APPEND cxx_sources
    "${cxx_sources_dir}/fake.cpp")

list(APPEND cxx_libraries
    ${Boost_LIBRARIES}
    ma_config)

add_library(${PROJECT_NAME}
    ${cxx_headers}
    ${cxx_sources})
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${cxx_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()
