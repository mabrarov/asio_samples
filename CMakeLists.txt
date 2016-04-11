#
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_samples)

# Assume MA_BUILD_TESTS=ON by default
if(NOT DEFINED MA_BUILD_TESTS)
    set(MA_BUILD_TESTS ON)
endif()

set(MA_BUILD_CMAKE "${PROJECT_SOURCE_DIR}/build/cmake")

# Setup path for additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${MA_BUILD_CMAKE}")

# Setup compiler & linker options
include(${MA_BUILD_CMAKE}/compiler_options.cmake)
include(${MA_BUILD_CMAKE}/linker_options.cmake)

# Additional helpers
include(${MA_BUILD_CMAKE}/functions.cmake)

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
if(MA_BUILD_TESTS)
    if(NOT MA_USE_OWN_GTEST)
        find_package(GTest QUIET)
    endif()
    if(MA_USE_OWN_GTEST OR NOT GTEST_FOUND)
        add_subdirectory(3rdparty/gtest)
        set(MA_OWN_GTEST ON)
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

add_subdirectory(ma_config)
add_subdirectory(${MA_BUILD_CMAKE}/asio_performance_test_client)
add_subdirectory(${MA_BUILD_CMAKE}/async_basics)
add_subdirectory(${MA_BUILD_CMAKE}/async_basics2)
add_subdirectory(${MA_BUILD_CMAKE}/async_connect)
add_subdirectory(${MA_BUILD_CMAKE}/echo_server)
add_subdirectory(${MA_BUILD_CMAKE}/nmea_client)
add_subdirectory(${MA_BUILD_CMAKE}/qt_echo_server)

if(MA_BUILD_TESTS)
    add_subdirectory(${MA_BUILD_CMAKE}/handler_storage_test)
    add_subdirectory(${MA_BUILD_CMAKE}/lockable_wrapper_test)
    add_subdirectory(${MA_BUILD_CMAKE}/shared_ptr_factory_test)
    add_subdirectory(${MA_BUILD_CMAKE}/sp_singleton_test)
    if(WIN32)
        add_subdirectory(${MA_BUILD_CMAKE}/windows_console_signal_test)
    endif()
endif()
