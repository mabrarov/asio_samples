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
# (use chono if available)
set(BOOST_COMPONENTS "system;thread;date_time;program_options;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

set(HEADERS
    "${HEADERS_DIR}/ma/echo/server/session_config_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_config_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_stats.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_stats_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_config.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_config.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/error.hpp"
    "${HEADERS_DIR}/ma/echo/server/session.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_manager_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/pooled_session_factory.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_factory.hpp"
    "${HEADERS_DIR}/ma/echo/server/session_factory_fwd.hpp"
    "${HEADERS_DIR}/ma/echo/server/simple_session_factory.hpp"
    "${HEADERS_DIR}/ma/detail/binder.hpp"
    "${HEADERS_DIR}/ma/detail/functional.hpp"
    "${HEADERS_DIR}/ma/detail/handler_ptr.hpp"
    "${HEADERS_DIR}/ma/detail/intrusive_list.hpp"            
    "${HEADERS_DIR}/ma/detail/latch.hpp"
    "${HEADERS_DIR}/ma/detail/memory.hpp"
    "${HEADERS_DIR}/ma/detail/service_base.hpp"
    "${HEADERS_DIR}/ma/detail/sp_singleton.hpp"
    "${HEADERS_DIR}/ma/detail/thread.hpp"
    "${HEADERS_DIR}/ma/detail/tuple.hpp"
    "${HEADERS_DIR}/ma/windows/console_signal.hpp"
    "${HEADERS_DIR}/ma/windows/console_signal_service.hpp"
    "${HEADERS_DIR}/ma/bind_handler.hpp"
    "${HEADERS_DIR}/ma/config.hpp"
    "${HEADERS_DIR}/ma/console_close_guard.hpp"
    "${HEADERS_DIR}/ma/context_alloc_handler.hpp"
    "${HEADERS_DIR}/ma/context_wrapped_handler.hpp"
    "${HEADERS_DIR}/ma/custom_alloc_handler.hpp"
    "${HEADERS_DIR}/ma/cyclic_buffer.hpp"            
    "${HEADERS_DIR}/ma/handler_alloc_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_allocator.hpp"
    "${HEADERS_DIR}/ma/handler_cont_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_invoke_helpers.hpp"
    "${HEADERS_DIR}/ma/handler_storage.hpp"
    "${HEADERS_DIR}/ma/handler_storage_service.hpp"
    "${HEADERS_DIR}/ma/limited_int.hpp"            
    "${HEADERS_DIR}/ma/shared_ptr_factory.hpp"
    "${HEADERS_DIR}/ma/sp_intrusive_list.hpp"
    "${HEADERS_DIR}/ma/steady_deadline_timer.hpp"
    "${HEADERS_DIR}/ma/strand.hpp"
    "${HEADERS_DIR}/ma/strand_wrapped_handler.hpp"            
    "${HEADERS_DIR}/ma/thread_group.hpp"            
    "${HEADERS_DIR}/ma/type_traits.hpp"
    "${SOURCES_DIR}/echo_server/config.hpp")

set(SOURCES
    "${SOURCES_DIR}/ma/echo/server/error.cpp"
    "${SOURCES_DIR}/ma/echo/server/session.cpp"
    "${SOURCES_DIR}/ma/echo/server/session_manager.cpp"
    "${SOURCES_DIR}/ma/echo/server/pooled_session_factory.cpp"
    "${SOURCES_DIR}/ma/echo/server/simple_session_factory.cpp"
    "${SOURCES_DIR}/ma/windows/console_signal_service.cpp"
    "${SOURCES_DIR}/ma/console_close_guard.cpp"
    "${SOURCES_DIR}/echo_server/config.cpp"
    "${SOURCES_DIR}/echo_server/main.cpp"
)

set(LIBRARIES ${Boost_LIBRARIES})

# Required Winsock libraries
if(WIN32)
    set(LIBRARIES "ws2_32;mswsock;${LIBRARIES}")    
endif()

# Boost chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    set(LIBRARIES "${LIBRARIES};rt")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})
add_executable(echo_server ${HEADERS} ${SOURCES})
target_link_libraries(echo_server ${LIBRARIES})
