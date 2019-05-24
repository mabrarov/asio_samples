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
set(boost_exception_header_only TRUE)

# Select used Boost libraries
find_package(Boost REQUIRED)
if(NOT(${Boost_VERSION} LESS 104700))
    list(APPEND boost_components exception)
    set(boost_has_exception TRUE)
endif()

if(boost_has_exception)
    # Boost.Exception is static library on Windows sometimes
    # https://www.boost.org/doc/libs/1_54_0/more/getting_started/windows.html#header-only-libraries
    if(WIN32 AND NOT(${Boost_VERSION} LESS 105400))
        set(Boost_USE_STATIC_LIBS ON)
        find_package(Boost COMPONENTS ${boost_components})
        if(Boost_FOUND)
            set(boost_exception_header_only FALSE)
            list(APPEND cxx_include_directories
                ${Boost_INCLUDE_DIRS})
            list(APPEND cxx_libraries
                ${Boost_LIBRARIES}
                ma_boost_link)
        endif()
    endif()
    if(boost_exception_header_only)
        list(APPEND cxx_libraries
            ma_boost_header_only
            ma_boost_link)
    endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${cxx_include_directories})
target_link_libraries(${PROJECT_NAME} INTERFACE ${cxx_libraries})
