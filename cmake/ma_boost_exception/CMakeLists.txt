#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_boost_exception)

set(cxx_include_directories )
set(cxx_libraries )
set(boost_components )
set(boost_has_exception FALSE)

# Select used Boost libraries
find_package(Boost REQUIRED)
if(NOT(${Boost_VERSION} LESS 104700))
    list(APPEND boost_components exception)
    set(boost_has_exception TRUE)
endif()

if(boost_has_exception)
    # Attach used Boost libraries
    # Boost.Exception is a static library always
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost REQUIRED COMPONENTS ${boost_components})
    list(APPEND cxx_include_directories
        ${Boost_INCLUDE_DIRS})
    list(APPEND cxx_libraries
        ${Boost_LIBRARIES}
        ma_boost_link)
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
