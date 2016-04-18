#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_boost_date_time)

set(cxx_include_directories )
set(cxx_libraries )
set(boost_components )

# Select used Boost libraries
list(APPEND boost_components date_time)

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${boost_components})
list(APPEND cxx_include_directories
    ${Boost_INCLUDE_DIRS})
list(APPEND cxx_libraries
    ${Boost_LIBRARIES}
    ma_boost_link)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
