#
# Copyright (c) 2018 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_context_invoke_handler_test)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_headers )
set(cxx_sources )

ma_config_public_compile_options(cxx_public_compile_options)
ma_config_public_compile_definitions(cxx_public_compile_definitions)
set(cxx_public_libraries )

ma_config_private_compile_options(cxx_private_compile_options)
ma_config_private_compile_definitions(cxx_private_compile_definitions)
set(cxx_private_libraries )

list(APPEND cxx_sources
    "${cxx_sources_dir}/context_invoke_handler_test.cpp")

list(APPEND cxx_private_libraries
    ma_boost_asio
    ma_gtest
    ma_config
    ma_compat
    ma_context_invoke_handler
    ma_coverage)

add_executable(${PROJECT_NAME}
    ${cxx_headers}
    ${cxx_sources})
target_compile_options(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_compile_options}
    PRIVATE
    ${cxx_private_compile_options})
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_compile_definitions}
    PRIVATE
    ${cxx_private_compile_definitions})
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_libraries}
    PRIVATE
    ${cxx_private_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
