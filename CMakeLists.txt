#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_samples)

set(ASIO_SAMPLES_BUILD_CMAKE "${PROJECT_SOURCE_DIR}/build/cmake")

# Setup path for additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ASIO_SAMPLES_BUILD_CMAKE}")

# Setup compiler & linker options
include(${ASIO_SAMPLES_BUILD_CMAKE}/compiler_options.cmake)
include(${ASIO_SAMPLES_BUILD_CMAKE}/linker_options.cmake)

# Additional helpers
include(${ASIO_SAMPLES_BUILD_CMAKE}/functions.cmake)

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

# Google test libraries
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
    add_subdirectory(3rdparty/gtest)
endif()

add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/asio_performance_test_client)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/async_basics)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/async_basics2)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/async_connect)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/echo_server)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/nmea_client)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/qt_echo_server)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/handler_storage_test)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/lockable_wrapper_test)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/shared_ptr_factory_test)
add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/sp_singleton_test)
if(WIN32)
    add_subdirectory(${ASIO_SAMPLES_BUILD_CMAKE}/windows_console_signal_test)
endif()
