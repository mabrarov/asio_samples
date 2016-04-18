#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_gtest)

set(cxx_compile_definitions )

if(NOT ma_own_gtest)
    find_package(GTest REQUIRED)
endif()

# MSVC 2012 doesn't work with Google Test and tuple - even with GTest own
# tuple implementation.
if(MSVC AND MSVC_VERSION GREATER 1600 AND MSVC_VERSION LESS 1800)
    list(APPEND cxx_compile_definitions GTEST_HAS_TR1_TUPLE=0)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME} INTERFACE ${GTEST_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${GTEST_BOTH_LIBRARIES})
