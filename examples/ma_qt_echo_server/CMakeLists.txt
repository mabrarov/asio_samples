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

config_public_compile_options(cxx_public_compile_options)
config_public_compile_definitions(cxx_public_compile_definitions)
set(cxx_public_libraries )

config_private_compile_options(cxx_private_compile_options)
config_private_compile_definitions(cxx_private_compile_definitions)
set(cxx_private_libraries )

# Determine major version of Qt to use (Qt 5 is preferred)
set(ma_qt_echo_server_qt5 FALSE)
if(ma_force_qt5)
    set(ma_qt_echo_server_qt5 TRUE)
else()
    find_package(Qt5Widgets QUIET)
    if(Qt5Widgets_FOUND)
        set(ma_qt_echo_server_qt5 TRUE)
    endif()
endif()

# Find required Qt libraries
set(qt_libraries )
if(ma_qt_echo_server_qt5)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Gui     REQUIRED)
    find_package(Qt5Core    REQUIRED)
    set(qt_libraries
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets)
else()
    find_package(Qt4 REQUIRED QtCore QtGui)
    set(qt_libraries
        Qt4::QtCore
        Qt4::QtGui)
endif()

# Support statically linked Qt
set(ma_qt_static FALSE)
if(ma_qt_echo_server_qt5)
    # Determine linkage type of Qt libraries (via linkage type of Qt Core library)
    get_target_property(qt5_core_type Qt5::Core TYPE)
    if(${qt5_core_type} STREQUAL "STATIC_LIBRARY")
        set(ma_qt_static TRUE)
    endif()

    if(ma_qt_static)
        # Statically linked Qt requires:
        #     All libraries used by Qt internally to be linked explicitly to the target.
        #     Used Qt plugins to be linked explicitly to the target.
        list(APPEND qt_libraries
            ma_qt5_core_static
            ma_qt5_gui_static)
    endif()
else()
    if(QT_IS_STATIC)
        # Statically linked Qt requires:
        #     All libraries used by Qt internally to be linked explicitly to the target.
        #     Used Qt plugins to be linked explicitly to the target.
        # Nothing to add as no Qt 4 plugins are used.
        set(ma_qt_static TRUE)
    endif()
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
    ma_echo_server_core)

# Wrap Qt generated sources.
set(moc_generated_sources )
set(uic_generated_headers )
set(qrc_generated_sources )
# Required for Qt UIC
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(ma_qt_echo_server_qt5)
    qt5_wrap_cpp(moc_generated_sources ${moc_headers})
    qt5_wrap_ui(uic_generated_headers ${uic_forms})
    qt5_add_resources(qrc_generated_sources ${qrc_resources})
else()
    qt4_wrap_cpp(moc_generated_sources ${moc_headers})
    qt4_wrap_ui(uic_generated_headers ${uic_forms})
    qt4_add_resources(qrc_generated_sources ${qrc_resources})
endif()

add_executable(${PROJECT_NAME} WIN32
    ${cxx_headers}
    ${cxx_sources}
    ${native_resources}
    ${native_resource_headers}
    ${moc_generated_sources}
    ${qrc_generated_sources}
    ${uic_generated_headers})
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

if(NOT ma_no_cmake_dir_source_group)
    # Group files according to file path
    ma_dir_source_group("Header Files" "${cxx_headers_dir}" "${cxx_headers}")
    ma_dir_source_group("Source Files" "${cxx_sources_dir}" "${cxx_sources}")
endif()