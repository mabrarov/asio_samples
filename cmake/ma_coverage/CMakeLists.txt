#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_coverage)

set(compile_options )
set(cxx_libraries )

if(ma_coverage_build)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND compile_options "-O0" "--coverage" "-fprofile-arcs" "-ftest-coverage")
        find_library(gcov_lib "gcov" DOC "Gcov library")
        if(NOT gcov_lib)
            message(FATAL_ERROR "Gcov library not found")
        endif()
        list(APPEND cxx_libraries gcov_lib)
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
target_compile_options(${PROJECT_NAME} INTERFACE ${compile_options})
