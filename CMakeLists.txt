#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(asio_samples)

option(ma_build_tests "Build tests" ON)
option(ma_force_own_gtest_usage "Use own (embedded) version of Google Test framework" OFF)

set(cmake_module_dir "${PROJECT_SOURCE_DIR}/cmake")

# Setup path for additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${cmake_module_dir})

# Setup compiler & linker options
include(${cmake_module_dir}/compiler_options.cmake)
include(${cmake_module_dir}/linker_options.cmake)

# Additional helpers
include(${cmake_module_dir}/functions.cmake)

# Additional preprocessor definitions for Windows target
if(WIN32)
    add_definitions(
        -DWIN32
        -DWIN32_LEAN_AND_MEAN
        -DWINVER=0x0501
        -D_WIN32_WINNT=0x0501
        -D_WIN32_WINDOWS=0x0501
        -D_WIN32_IE=0x0600
        -D_UNICODE
        -DUNICODE
        -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

# Google Test framework
if(ma_build_tests)
    if(NOT ma_force_own_gtest_usage)
        find_package(GTest QUIET)
    endif()
    if(ma_force_own_gtest_usage OR NOT GTEST_FOUND)
        add_subdirectory(3rdparty/gtest)
        set(ma_own_gtest ON)
        set(GTEST_LIBRARIES        gtest)
        set(GTEST_MAIN_LIBRARIES   gtest_main)
        set(GTEST_BOTH_LIBRARIES   gtest gtest_main)
        get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
        # Workaround for old version of Google Test
        if(NOT GTEST_INCLUDE_DIRS)
            set(GTEST_INCLUDE_DIRS ${gtest_SOURCE_DIR}/include)
        endif()
    endif()
endif()

add_subdirectory(libs/ma_async_connect)
add_subdirectory(libs/ma_bind_handler)
add_subdirectory(libs/ma_codecvt_cast)
add_subdirectory(libs/ma_compat)
add_subdirectory(libs/ma_config)
add_subdirectory(libs/ma_console_close_signal)
add_subdirectory(libs/ma_context_alloc_handler)
add_subdirectory(libs/ma_context_invoke_handler)
add_subdirectory(libs/ma_context_wrapped_handler)
add_subdirectory(libs/ma_custom_alloc_handler)
add_subdirectory(libs/ma_cyclic_buffer)
add_subdirectory(libs/ma_handler_ptr)
add_subdirectory(libs/ma_handler_storage)
add_subdirectory(libs/ma_helpers)
add_subdirectory(libs/ma_intrusive_list)
add_subdirectory(libs/ma_io_service_pool)
add_subdirectory(libs/ma_limited_int)
add_subdirectory(libs/ma_lockable_wrapped_handler)
add_subdirectory(libs/ma_service_base)
add_subdirectory(libs/ma_shared_ptr_factory)
add_subdirectory(libs/ma_sp_intrusive_list)
add_subdirectory(libs/ma_sp_singleton)
add_subdirectory(libs/ma_steady_deadline_timer)
add_subdirectory(libs/ma_strand)
add_subdirectory(libs/ma_thread_group)
add_subdirectory(libs/ma_windows_console_signal)

if(ma_build_tests)
    add_subdirectory(test/ma_async_connect_test)
    add_subdirectory(test/ma_handler_storage_test)
    add_subdirectory(test/ma_lockable_wrapper_test)
    add_subdirectory(test/ma_shared_ptr_factory_test)
    add_subdirectory(test/ma_sp_singleton_test)
    if(WIN32)
        add_subdirectory(test/ma_windows_console_signal_test)
    endif()
endif()

add_subdirectory(examples/ma_asio_performance_test_client)
add_subdirectory(examples/ma_async_basics)
add_subdirectory(examples/ma_async_basics2)
add_subdirectory(examples/ma_echo_server)
add_subdirectory(examples/ma_echo_server_core)
add_subdirectory(examples/ma_nmea_client)
add_subdirectory(examples/ma_qt_echo_server)
