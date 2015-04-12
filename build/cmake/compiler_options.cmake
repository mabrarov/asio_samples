# 
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)

# Turn on pthread usage
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

# Turn on support of C++11 if it's available
if(CMAKE_COMPILER_IS_GNUCXX)
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7"))
        set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    elseif(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.3"))
        set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
    endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14"))
        set(CMAKE_CXX_FLAGS "/Qstd=c++11 ${CMAKE_CXX_FLAGS}")
    elseif(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12"))
        set(CMAKE_CXX_FLAGS "/Qstd=c++0x ${CMAKE_CXX_FLAGS}")
    endif()
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
