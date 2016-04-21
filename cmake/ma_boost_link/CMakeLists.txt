#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_boost_link)

set(cxx_compile_definitions )

# Turn off Boost autolinking
list(APPEND cxx_compile_definitions "BOOST_ALL_NO_LIB")

add_library(${PROJECT_NAME} INTERFACE)
target_compile_definitions(${PROJECT_NAME} INTERFACE ${cxx_compile_definitions})
