# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8)
project(asio_samples)

# Set additional options for Windows targets
if(WIN32)
    add_definitions(
        -DWIN32_LEAN_AND_MEAN
        -D_UNICODE
        -DUNICODE
        -DWINVER=0x0500
        -D_WIN32_WINNT=0x0500)
endif()

add_subdirectory(asio_performance_test_client)
add_subdirectory(async_basics)
add_subdirectory(async_basics2)
add_subdirectory(async_connect)
add_subdirectory(echo_server)
add_subdirectory(nmea_client)
add_subdirectory(shared_ptr_factory_test)
add_subdirectory(sp_singleton_test)

if(WIN32)
    add_subdirectory(windows_console_signal_test)
endif()
