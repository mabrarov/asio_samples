# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(qt_echo_server)

# Just determine version of Boost C++ Libraries
find_package(Boost REQUIRED)

# Build list of used Boost libraries depending on Boost veesion 
# (use chono if available)
set(BOOST_COMPONENTS "system;thread;date_time;regex")
if(NOT(${Boost_VERSION} LESS 104700))
    set(BOOST_COMPONENTS "${BOOST_COMPONENTS};chrono")
    set(BOOST_HAS_CHRONO TRUE)
else()
    set(BOOST_HAS_CHRONO FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Find required Qt libraries
find_package(Qt5Core    REQUIRED)
find_package(Qt5Gui     REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(HEADERS_DIR "${PROJECT_SOURCE_DIR}/../../../include")
set(SOURCES_DIR "${PROJECT_SOURCE_DIR}/../../../src")

set(MOC_HEADERS
    "${HEADERS_DIR}/ma/echo/server/qt/mainform.h"
    "${HEADERS_DIR}/ma/echo/server/qt/service.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceforwardsignal.h"
    "${HEADERS_DIR}/ma/echo/server/qt/serviceservantsignal.h")

set(UIC_FORMS
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.ui")

set(RCC_RESOURCES
    )

set(SOURCES 
    "${SOURCES_DIR}/ma/echo/server/qt/custommetatypes.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/service.cpp"
    "${SOURCES_DIR}/ma/echo/server/qt/mainform.cpp"
    "${SOURCES_DIR}/ma/echo/server/error.cpp"
    "${SOURCES_DIR}/ma/echo/server/session.cpp"
    "${SOURCES_DIR}/ma/echo/server/session_manager.cpp"
    "${SOURCES_DIR}/ma/echo/server/pooled_session_factory.cpp"
    "${SOURCES_DIR}/ma/echo/server/simple_session_factory.cpp"
    "${SOURCES_DIR}/qt_echo_server/main.cpp"
    "${SOURCES_DIR}/qt_echo_server/qt_echo_server.rc"
    ${UIC_FORMS})

set(LIBRARIES "${Boost_LIBRARIES};Qt5::Core;Qt5::Gui;Qt5::Widgets")

# Required Winsock libraries
if(WIN32)
    set(LIBRARIES "ws2_32;mswsock;comdlg32;uuid;ole32;oleaut32;advapi32;imm32;winmm;winspool;${LIBRARIES}")    
endif()

# Boost chrono requires rt library on *nix
if(UNIX AND BOOST_HAS_CHRONO)
    set(LIBRARIES "rt;${LIBRARIES}")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Wrap Qt generated sources.
qt5_wrap_cpp(MOC_GENERATED_SOURCES ${MOC_HEADERS})
qt5_wrap_ui(UIC_GENERATED_HEADERS ${UIC_FORMS})
qt5_add_resources(RCC_GENERATED_SOURCES ${RCC_RESOURCES})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_DIR})
include_directories(${UIC_GENERATED_HEADERS})
add_executable(qt_echo_server WIN32 ${SOURCES} ${MOC_GENERATED_SOURCES} ${RCC_GENERATED_SOURCES})
target_link_libraries(qt_echo_server ${LIBRARIES})
