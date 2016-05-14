#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_gtest)

set(cxx_compile_definitions )
set(cxx_include_directories )
set(cxx_libraries )

if(NOT ma_own_gtest)
    find_package(GTest REQUIRED)
endif()

if(MSVC)
    if((MSVC_VERSION EQUAL 1800) OR (MSVC_VERSION GREATER 1800))
        # MSVC provides enough support for std::tuple starting from MSVC 2013 
        # and GTEST_LANG_CXX11 is used by GTest just for checking avaliablility 
        # of std::tuple
        list(APPEND cxx_compile_definitions GTEST_LANG_CXX11=1)        
    elseif((MSVC_VERSION EQUAL 1700) OR (MSVC_VERSION GREATER 1700))
        # MSVC 2012 std::tuple doesn't work with Google Test. 
        # Even GTest own tuple implementation doesn't help.
        list(APPEND cxx_compile_definitions GTEST_HAS_TR1_TUPLE=0)
    elseif(((MSVC_VERSION EQUAL 1500) OR (MSVC_VERSION GREATER 1500)) AND MSVC_VERSION LESS 1600)
        # MSVC 2008 SP1 has TR1 tuple which is compatible with GTest
        list(APPEND cxx_compile_definitions GTEST_USE_OWN_TR1_TUPLE=0)
    endif()
endif()

list(APPEND cxx_include_directories
    ${GTEST_INCLUDE_DIRS})
list(APPEND cxx_libraries
    ${GTEST_BOTH_LIBRARIES})

add_library(${PROJECT_NAME} INTERFACE)
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
