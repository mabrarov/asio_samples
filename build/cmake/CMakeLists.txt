# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_samples)

# Setup path for additional CMake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

# Setup compiler & linker options
include(compiler_options.cmake)
include(linker_options.cmake)

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

add_subdirectory(asio_performance_test_client)
add_subdirectory(async_basics)
add_subdirectory(async_basics2)
add_subdirectory(async_connect)
add_subdirectory(echo_server)
add_subdirectory(handler_storage_test)
add_subdirectory(nmea_client)
add_subdirectory(qt_echo_server)
add_subdirectory(shared_ptr_factory_test)
add_subdirectory(sp_singleton_test)

if(WIN32)
    add_subdirectory(windows_console_signal_test)
endif()
