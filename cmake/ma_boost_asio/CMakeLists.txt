#
# Copyright (c) 2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_boost_asio)

set(cxx_libraries )

list(APPEND cxx_libraries
    ma_boost_header_only
    ma_boost_system
    ma_boost_date_time
    ma_boost_chrono
    ma_boost_regex
    ma_boost_link)

# Add platform libraries required for Asio
if(WIN32)
    list(APPEND cxx_libraries "ws2_32" "mswsock")
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
