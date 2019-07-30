#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_coverage)

set(compile_options )
set(link_options )
set(cxx_libraries )

if(MA_COVERAGE)
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND compile_options "-O0" "-fprofile-arcs" "-ftest-coverage")
        list(APPEND link_options "--coverage")
    endif()
endif()

set(link_options_linear )
set(first_link_option TRUE)
foreach(link_option IN LISTS link_options)
    if(first_link_option)
        set(link_options_linear "${link_option}")
    else()
        set(link_options_linear "${link_options_linear},${link_option}")
    endif()
    set(first_link_option FALSE)
endforeach()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries} "${link_options_linear}")
target_compile_options(${PROJECT_NAME} INTERFACE ${compile_options})
