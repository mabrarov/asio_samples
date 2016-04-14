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

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_public_libraries )
set(cxx_private_libraries )
set(boost_components )

# Select used Boost libraries
find_package(Boost REQUIRED)
list(APPEND boost_components
    system
    date_time
    regex)
if(NOT(${Boost_VERSION} LESS 104700))
    list(APPEND boost_components
        chrono)
    set(ma_boost_has_chrono TRUE)
else()
    set(ma_boost_has_chrono FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${boost_components})

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()

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
    ma_steady_deadline_timer
    ma_strand
    ma_handler_storage
    ma_limited_int
    ma_sp_intrusive_list
    ma_intrusive_list
    ma_cyclic_buffer
    ma_context_alloc_handler
    ma_custom_alloc_handler
    ma_bind_handler
    ma_compat
    ma_config
    ${Boost_LIBRARIES})

list(APPEND cxx_private_libraries
    ma_shared_ptr_factory)

# Required Winsock libraries
if(WIN32)
    list(APPEND cxx_private_libraries
        "ws2_32"
        "mswsock")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND ma_boost_has_chrono)
    list(APPEND cxx_private_libraries
        rt)
endif()

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
    ${cxx_public_libraries}
    PRIVATE
    ${cxx_private_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()
