#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(ma_qt_echo_server)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_headers )
set(cxx_sources )
set(uic_forms )
set(qrc_resources )
set(moc_headers )
set(native_resources )
set(native_resource_headers )
set(qt_msvs_project_support_files )

config_public_compile_options(cxx_public_compile_options)
config_public_compile_definitions(cxx_public_compile_definitions)
set(cxx_public_libraries )

config_private_compile_options(cxx_private_compile_options)
config_private_compile_definitions(cxx_private_compile_definitions)
set(cxx_private_libraries )

# Determine major version of Qt to use (Qt 5 is preferred)
set(ma_qt_echo_server_qt_major_version )
if(ma_force_qt_major_version)
    set(ma_qt_echo_server_qt_major_version ${ma_force_qt_major_version})
else()
    find_package(Qt5Widgets QUIET)
    if(Qt5Widgets_FOUND)
        set(ma_qt_echo_server_qt_major_version 5)
    else()
        set(ma_qt_echo_server_qt_major_version 4)
    endif()
endif()

# Find required Qt libraries
set(qt_libraries )
if(ma_qt_echo_server_qt_major_version EQUAL 5)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Gui     REQUIRED)
    find_package(Qt5Core    REQUIRED)
    set(qt_libraries
        Qt5::Widgets
        Qt5::Gui
        Qt5::Core
        ma_qt5_widgets_support
        ma_qt5_gui_support
        ma_qt5_core_support)
elseif(ma_qt_echo_server_qt_major_version EQUAL 4)
    find_package(Qt4 REQUIRED QtCore QtGui)
    set(qt_libraries
        Qt4::QtGui
        Qt4::QtCore
        ma_qt4_gui_support
        ma_qt4_core_support)
else()
    message(FATAL_ERROR "Unknown version of Qt: " ${ma_qt_echo_server_qt_major_version})
endif()

list(APPEND uic_forms
    "${cxx_sources_dir}/mainform.ui")

list(APPEND moc_headers
    "${cxx_sources_dir}/mainform.h"
    "${cxx_sources_dir}/service.h"
    "${cxx_sources_dir}/serviceforwardsignal.h"
    "${cxx_sources_dir}/serviceservantsignal.h")

list(APPEND cxx_headers
    "${cxx_sources_dir}/custommetatypes.h"
    "${cxx_sources_dir}/execution_config.h"
    "${cxx_sources_dir}/execution_config_fwd.h"
    "${cxx_sources_dir}/serviceforwardsignal_fwd.h"
    "${cxx_sources_dir}/serviceservantsignal_fwd.h"
    "${cxx_sources_dir}/servicestate.h"
    "${cxx_sources_dir}/service_fwd.h"
    "${cxx_sources_dir}/meta_type_register_error.h"
    "${cxx_sources_dir}/signal_connect_error.h"
    ${moc_headers})

list(APPEND cxx_sources
    "${cxx_sources_dir}/custommetatypes.cpp"
    "${cxx_sources_dir}/service.cpp"
    "${cxx_sources_dir}/mainform.cpp"
    "${cxx_sources_dir}/main.cpp")

if(WIN32)
    list(APPEND native_resources
        "${cxx_sources_dir}/qt_echo_server.rc")
endif()

list(APPEND cxx_private_libraries
    ma_boost_header_only
    ma_boost_exception
    ma_boost_system
    ma_boost_asio
    ${qt_libraries}
    ma_compat
    ma_helpers
    ma_thread_group
    ma_echo_server_core
    ma_coverage)

# Wrap Qt generated sources.
set(moc_generated_sources )
set(uic_generated_headers )
set(qrc_generated_sources )
# Required for Qt UIC
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(ma_qt_echo_server_qt_major_version EQUAL 5)
    qt5_wrap_cpp(moc_generated_sources ${moc_headers})
    qt5_wrap_ui(uic_generated_headers ${uic_forms})
    qt5_add_resources(qrc_generated_sources ${qrc_resources})
elseif(ma_qt_echo_server_qt_major_version EQUAL 4)
    qt4_wrap_cpp(moc_generated_sources ${moc_headers})
    qt4_wrap_ui(uic_generated_headers ${uic_forms})
    qt4_add_resources(qrc_generated_sources ${qrc_resources})
endif()

if(ma_qt_echo_server_qt_major_version EQUAL 5)
    list(APPEND qt_msvs_project_support_files
        ${ma_qt5_msvs_project_support_files})
elseif(ma_qt_echo_server_qt_major_version EQUAL 4)
    list(APPEND qt_msvs_project_support_files
        ${ma_qt4_msvs_project_support_files})
endif()

add_executable(${PROJECT_NAME} WIN32
    ${cxx_headers}
    ${cxx_sources}
    ${native_resources}
    ${native_resource_headers}
    ${moc_generated_sources}
    ${qrc_generated_sources}
    ${uic_generated_headers}
    ${qt_msvs_project_support_files})
target_compile_options(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_compile_options}
    PRIVATE
    ${cxx_private_compile_options})
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_compile_definitions}
    PRIVATE
    ${cxx_private_compile_definitions})
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${cxx_public_libraries}
    PRIVATE
    ${cxx_private_libraries})

source_group("Form Files" FILES
    ${uic_forms})
source_group("Generated Files" FILES
    ${moc_generated_sources}
    ${qrc_generated_sources}
    ${uic_generated_headers})
source_group("Resource Files" FILES
    ${native_resources}
    ${native_resource_headers}
    ${qrc_resources})
source_group("Project Support Files" FILES
    ${qt_msvs_project_support_files})

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()
