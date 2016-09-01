#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_echo_server_core)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_headers )
set(cxx_sources )

config_public_compile_options(cxx_public_compile_options)
config_public_compile_definitions(cxx_public_compile_definitions)
set(cxx_public_libraries )

config_private_compile_options(cxx_private_compile_options)
config_private_compile_definitions(cxx_private_compile_definitions)
set(cxx_private_libraries )

list(APPEND cxx_headers
    "${cxx_headers_dir}/ma/echo/server/session_config_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_config_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_stats.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_stats_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_config.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_config.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/error.hpp"
    "${cxx_headers_dir}/ma/echo/server/session.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_manager_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/pooled_session_factory.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_factory.hpp"
    "${cxx_headers_dir}/ma/echo/server/session_factory_fwd.hpp"
    "${cxx_headers_dir}/ma/echo/server/simple_session_factory.hpp")

list(APPEND cxx_sources
    "${cxx_sources_dir}/error.cpp"
    "${cxx_sources_dir}/session.cpp"
    "${cxx_sources_dir}/session_manager.cpp"
    "${cxx_sources_dir}/pooled_session_factory.cpp"
    "${cxx_sources_dir}/simple_session_factory.cpp")

list(APPEND cxx_public_libraries
    ma_boost_header_only
    ma_boost_asio
    ma_boost_system
    ma_boost_date_time
    ma_config
    ma_compat
    ma_bind_handler
    ma_custom_alloc_handler
    ma_context_alloc_handler
    ma_cyclic_buffer
    ma_intrusive_list
    ma_sp_intrusive_list
    ma_limited_int
    ma_handler_storage
    ma_strand
    ma_steady_deadline_timer)

list(APPEND cxx_private_libraries
    ma_shared_ptr_factory
    ma_coverage)

add_library(${PROJECT_NAME}
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
