#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_samples)

# Assume ma_build_tests=ON by default
if(NOT DEFINED ma_build_tests)
    set(ma_build_tests ON)
endif()

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
    if(NOT MA_USE_OWN_GTEST)
        find_package(GTest QUIET)
    endif()
    if(MA_USE_OWN_GTEST OR NOT GTEST_FOUND)
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

#add_subdirectory(ma_asio_performance_test_client)
#add_subdirectory(ma_async_basics)
#add_subdirectory(ma_async_basics2)
#add_subdirectory(ma_async_connect)
add_subdirectory(ma_bind_handler)
#add_subdirectory(ma_codecvt_cast)
add_subdirectory(ma_compat)
add_subdirectory(ma_config)
#add_subdirectory(ma_console_close_signal)
#add_subdirectory(ma_context_alloc_handler)
#add_subdirectory(ma_context_invoke_handler)
add_subdirectory(ma_context_wrapped_handler)
add_subdirectory(ma_custom_alloc_handler)
add_subdirectory(ma_cyclic_buffer)
#add_subdirectory(ma_echo_server)
#add_subdirectory(ma_echo_server_core)
#add_subdirectory(ma_handler_ptr)
#add_subdirectory(ma_handler_storage)
add_subdirectory(ma_helpers)
#add_subdirectory(ma_intrusive_list)
#add_subdirectory(ma_io_service_pool)
#add_subdirectory(ma_limited_int)
#add_subdirectory(ma_lockable_wrapped_handler)
#add_subdirectory(ma_nmea_client)
#add_subdirectory(ma_qt_echo_server)
#add_subdirectory(ma_shared_ptr_factory)
#add_subdirectory(ma_sp_intrusive_list)
#add_subdirectory(ma_sp_singleton)
#add_subdirectory(ma_steady_deadline_timer)
#add_subdirectory(ma_strand)
#add_subdirectory(ma_thread_group)
#add_subdirectory(ma_windows_console_signal)

#if(ma_build_tests)
#    add_subdirectory(test)
#endif()
