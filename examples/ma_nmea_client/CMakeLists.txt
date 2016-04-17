#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_nmea_client)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_compile_options     ${cxx_base_compile_options})
set(cxx_compile_definitions ${cxx_base_compile_definitions})
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )
set(boost_components )

# Select used Boost libraries
find_package(Boost REQUIRED)
list(APPEND boost_components
    system
    date_time
    regex
    thread)
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
    "${cxx_sources_dir}/cyclic_read_session_fwd.hpp"
    "${cxx_sources_dir}/cyclic_read_session.hpp"
    "${cxx_sources_dir}/error.hpp"
    "${cxx_sources_dir}/frame.hpp")

list(APPEND cxx_sources
    "${cxx_sources_dir}/error.cpp"
    "${cxx_sources_dir}/cyclic_read_session.cpp"
    "${cxx_sources_dir}/main.cpp")

list(APPEND cxx_libraries
    ma_codecvt_cast
    ma_console_close_signal
    ma_thread_group
    ma_strand
    ma_handler_storage
    ma_context_alloc_handler
    ma_custom_alloc_handler
    ma_helpers
    ma_bind_handler
    ma_shared_ptr_factory
    ma_compat
    ma_config
    ${Boost_LIBRARIES})

# Required Winsock libraries
if(WIN32)
    list(APPEND cxx_libraries
        "ws2_32"
        "mswsock")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND ma_boost_has_chrono)
    list(APPEND cxx_libraries
        rt)
endif()

add_executable(${PROJECT_NAME}
    ${cxx_headers}
    ${cxx_sources})
target_compile_options(${PROJECT_NAME}
    PRIVATE
    ${cxx_compile_options})
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${cxx_libraries})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()
