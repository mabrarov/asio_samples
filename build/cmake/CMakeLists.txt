# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(asio_samples)

# Set additional options for Windows targets
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

# Turn on support of C++11 if it's available
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "/Qstd=c++0x ${CMAKE_CXX_FLAGS}")    
endif()

# Turn on more strict warning mode
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
elseif(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
endif()

# Turn on Unicode support for MinGW for main function
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-municode ${CMAKE_EXE_LINKER_FLAGS}")
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
