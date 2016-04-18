#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_boost_asio)

set(cxx_libraries )
set(boost_components )
set(boost_has_chrono FALSE)

# Select used Boost libraries
list(APPEND boost_components system date_time regex)
find_package(Boost REQUIRED)
if(NOT(${Boost_VERSION} LESS 104700))
    list(APPEND boost_components chrono)
    set(boost_has_chrono TRUE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${boost_components})
list(APPEND cxx_libraries ${Boost_LIBRARIES} ma_boost_link)

# Add required platform libraries
if(WIN32)
    list(APPEND cxx_libraries "ws2_32" "mswsock")
endif()
if(UNIX AND boost_has_chrono)
    # Boost.Chrono requires rt library on *nix
    list(APPEND cxx_libraries "rt")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
