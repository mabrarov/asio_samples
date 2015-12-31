#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(echo_server)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Build list of used Boost libraries depending on Boost version
# (use Boost.Chrono if available)
set(BOOST_COMPONENTS "system;thread;date_time;program_options;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

set(PROJECT_BASE_DIR "${PROJECT_SOURCE_DIR}/../../..")
set(SOURCES_DIR      "${PROJECT_BASE_DIR}/src")
set(CPP_HEADERS_DIR  "${PROJECT_BASE_DIR}/include")

set(CPP_HEADERS
    "${CPP_HEADERS_DIR}/ma/echo/server/session_config_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_config_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_stats.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_stats_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_config.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_config.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/error.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/pooled_session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/session_factory_fwd.hpp"
    "${CPP_HEADERS_DIR}/ma/echo/server/simple_session_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/binder.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/functional.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/handler_ptr.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/intrusive_list.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/latch.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/memory.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/service_base.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/sp_singleton.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/thread.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/tuple.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/utility.hpp"
    "${CPP_HEADERS_DIR}/ma/windows/console_signal.hpp"
    "${CPP_HEADERS_DIR}/ma/windows/console_signal_service.hpp"
    "${CPP_HEADERS_DIR}/ma/bind_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/config.hpp"
    "${CPP_HEADERS_DIR}/ma/console_close_signal.hpp"
    "${CPP_HEADERS_DIR}/ma/console_close_signal_service.hpp"
    "${CPP_HEADERS_DIR}/ma/context_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/cyclic_buffer.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_allocator.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage.hpp"
    "${CPP_HEADERS_DIR}/ma/handler_storage_service.hpp"
    "${CPP_HEADERS_DIR}/ma/limited_int.hpp"
    "${CPP_HEADERS_DIR}/ma/shared_ptr_factory.hpp"
    "${CPP_HEADERS_DIR}/ma/sp_intrusive_list.hpp"
    "${CPP_HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${CPP_HEADERS_DIR}/ma/strand.hpp"
    "${CPP_HEADERS_DIR}/ma/strand_wrapped_handler.hpp"
    "${CPP_HEADERS_DIR}/ma/thread_group.hpp"
    "${CPP_HEADERS_DIR}/ma/detail/type_traits.hpp"
    "${SOURCES_DIR}/echo_server/config.hpp")

set(CPP_SOURCES
    "${SOURCES_DIR}/ma/echo/server/error.cpp"
    "${SOURCES_DIR}/ma/echo/server/session.cpp"
    "${SOURCES_DIR}/ma/echo/server/session_manager.cpp"
    "${SOURCES_DIR}/ma/echo/server/pooled_session_factory.cpp"
    "${SOURCES_DIR}/ma/echo/server/simple_session_factory.cpp"
    "${SOURCES_DIR}/ma/windows/console_signal_service.cpp"
    "${SOURCES_DIR}/echo_server/config.cpp"
    "${SOURCES_DIR}/echo_server/main.cpp")

set(LIBRARIES ${Boost_LIBRARIES})

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

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CPP_HEADERS_DIR})
add_executable(${PROJECT_NAME} ${CPP_HEADERS} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

if(NOT NO_CMAKE_DIR_SOURCE_GROUP)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${CPP_HEADERS_DIR}" "${CPP_HEADERS}")
    ma_dir_source_group("Source Files" "${SOURCES_DIR}" "${CPP_SOURCES}")
endif()
