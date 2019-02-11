#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 3.0)
project(asio_samples)

option(ma_build_tests "Build tests" ON)
option(ma_force_own_gtest "Use own (embedded) version of Google Test framework" OFF)
option(ma_qt "Use Qt and do not skip all the code requiring Qt" ON)
option(ma_coverage_build "Add coverage flags for compiler and linker" OFF)

# Use ma_force_qt_major_version to force usage of Qt 5.x or Qt 4.x:
# -D ma_force_qt_major_version=4
# or
# -D ma_force_qt_major_version=5

# Additional helpers
include(cmake/internal_utils.cmake)

# Get full list of build configurations
set(configuration_types ${CMAKE_CONFIGURATION_TYPES})
if(NOT configuration_types AND DEFINED CMAKE_BUILD_TYPE)
    list(APPEND configuration_types ${CMAKE_BUILD_TYPE})
endif()

# Common target properties (CMAKE_CXX_FLAGS)
ma_change_default_compile_options("${CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)
foreach(configuration_type IN LISTS configuration_types)
    string(TOUPPER ${configuration_type} configuration_type_upper_case)
    set(configuration_cxx_flags CMAKE_CXX_FLAGS_${configuration_type_upper_case})
    ma_change_default_compile_options("${${configuration_cxx_flags}}" ${configuration_cxx_flags})
endforeach(configuration_type)

# Common target properties (CMAKE_EXE_LINKER_FLAGS)
ma_change_default_link_options("${CMAKE_EXE_LINKER_FLAGS}" CMAKE_EXE_LINKER_FLAGS)
foreach(configuration_type IN LISTS configuration_types)
    string(TOUPPER ${configuration_type} configuration_type_upper_case)
    set(configuration_cxx_flags CMAKE_EXE_LINKER_FLAGS_${configuration_type_upper_case})
    ma_change_default_link_options("${${configuration_cxx_flags}}" ${configuration_cxx_flags})
endforeach(configuration_type)

# Turn on grouping of projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Groups of projects reflect location of projects in file system
set(project_group_3rdparty "3rdparty")
set(project_group_libs "libs")
set(project_group_tests "tests")
set(project_group_examples "examples")

# Interface-only target for turning on code coverage
add_subdirectory(cmake/ma_coverage)

# Google Test framework
if(ma_build_tests)
    if(NOT ma_force_own_gtest)
        find_package(GTest QUIET)
    endif()
    if(ma_force_own_gtest OR NOT GTEST_FOUND)
        add_subdirectory(3rdparty/gtest)
        set_target_properties(gtest PROPERTIES FOLDER "${project_group_3rdparty}/gtest")
        set_target_properties(gtest_main PROPERTIES FOLDER "${project_group_3rdparty}/gtest")
        set(ma_own_gtest ON)
        set(GTEST_LIBRARIES        gtest)
        set(GTEST_MAIN_LIBRARIES   gtest_main)
        set(GTEST_BOTH_LIBRARIES   gtest gtest_main)
        get_target_property(GTEST_INCLUDE_DIRS gtest INTERFACE_INCLUDE_DIRECTORIES)
        # Workaround for old version of Google Test
        if(NOT GTEST_INCLUDE_DIRS)
            set(GTEST_INCLUDE_DIRS ${gtest_SOURCE_DIR}/include)
        endif()
    endif()
    enable_testing()
endif()

# Interface-only target for turning on position idependent code compiler flag
add_subdirectory(cmake/ma_pic_on)

# Interface-only targets (wrappers) to simplify support of Boost
add_subdirectory(cmake/ma_boost_link)
add_subdirectory(cmake/ma_boost_header_only)
add_subdirectory(cmake/ma_boost_exception)
add_subdirectory(cmake/ma_boost_system)
add_subdirectory(cmake/ma_boost_date_time)
add_subdirectory(cmake/ma_boost_chrono)
add_subdirectory(cmake/ma_boost_thread)
add_subdirectory(cmake/ma_boost_asio)
add_subdirectory(cmake/ma_boost_program_options)
add_subdirectory(cmake/ma_boost_timer)
add_subdirectory(cmake/ma_boost_regex)

set(ma_qt4_msvs_project_support_files )
set(ma_qt5_msvs_project_support_files )
if(ma_qt)
    # Interface-only targets to simplify support of Qt 5
    add_subdirectory(cmake/ma_qt5_core_support)
    add_subdirectory(cmake/ma_qt5_gui_support)
    add_subdirectory(cmake/ma_qt5_widgets_support)
    # Interface-only targets to simplify support of Qt 4
    add_subdirectory(cmake/ma_qt4_core_support)
    add_subdirectory(cmake/ma_qt4_gui_support)

    # MSVS project support files for Qt.
    # Starting from MSVS 2015 these files can be included into MSVS project.
    if(${CMAKE_GENERATOR} MATCHES "Visual Studio 14")
        list(APPEND ma_qt4_msvs_project_support_files
            "${CMAKE_SOURCE_DIR}/cmake/qt_msvs_tools/qt4.natvis")
        list(APPEND ma_qt5_msvs_project_support_files
            "${CMAKE_SOURCE_DIR}/cmake/qt_msvs_tools/qt5.natvis")
    endif()
endif()

if(ma_build_tests)
    # Interface-only target to simplify support of Google Test
    add_subdirectory(cmake/ma_gtest)
endif()

# Libraries
add_subdirectory(libs/ma_async_connect)
set_target_properties(ma_async_connect PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_bind_handler)
set_target_properties(ma_bind_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_codecvt_cast)
set_target_properties(ma_codecvt_cast PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_compat)
set_target_properties(ma_compat PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_config)
set_target_properties(ma_config PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_console_close_signal)
set_target_properties(ma_console_close_signal PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_context_alloc_handler)
set_target_properties(ma_context_alloc_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_context_invoke_handler)
set_target_properties(ma_context_invoke_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_context_wrapped_handler)
set_target_properties(ma_context_wrapped_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_custom_alloc_handler)
set_target_properties(ma_custom_alloc_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_cyclic_buffer)
set_target_properties(ma_cyclic_buffer PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_handler_ptr)
set_target_properties(ma_handler_ptr PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_handler_storage)
set_target_properties(ma_handler_storage PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_helpers)
set_target_properties(ma_helpers PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_intrusive_list)
set_target_properties(ma_intrusive_list PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_io_service_pool)
set_target_properties(ma_io_service_pool PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_limited_int)
set_target_properties(ma_limited_int PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_lockable_wrapped_handler)
set_target_properties(ma_lockable_wrapped_handler PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_service_base)
set_target_properties(ma_service_base PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_shared_ptr_factory)
set_target_properties(ma_shared_ptr_factory PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_sp_intrusive_list)
set_target_properties(ma_sp_intrusive_list PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_sp_singleton)
set_target_properties(ma_sp_singleton PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_steady_deadline_timer)
set_target_properties(ma_steady_deadline_timer PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_strand)
set_target_properties(ma_strand PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_thread_group)
set_target_properties(ma_thread_group PROPERTIES FOLDER "${project_group_libs}")

add_subdirectory(libs/ma_windows_console_signal)
set_target_properties(ma_windows_console_signal PROPERTIES FOLDER "${project_group_libs}")

# Tests
if(ma_build_tests)
    add_subdirectory(tests/ma_async_connect_test)
    set_target_properties(ma_async_connect_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_handler_storage_test)
    set_target_properties(ma_handler_storage_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_lockable_wrapper_test)
    set_target_properties(ma_lockable_wrapper_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_shared_ptr_factory_test)
    set_target_properties(ma_shared_ptr_factory_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_sp_singleton_test)
    set_target_properties(ma_sp_singleton_test PROPERTIES FOLDER "${project_group_tests}")

    if(WIN32)
        add_subdirectory(tests/ma_windows_console_signal_test)
        set_target_properties(ma_windows_console_signal_test PROPERTIES FOLDER "${project_group_tests}")
    endif()

    add_subdirectory(tests/ma_handler_allocator_test)
    set_target_properties(ma_handler_allocator_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_custom_alloc_handler_test)
    set_target_properties(ma_custom_alloc_handler_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_cyclic_buffer_test)
    set_target_properties(ma_cyclic_buffer_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_sp_intrusive_list_test)
    set_target_properties(ma_sp_intrusive_list_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_strand_test)
    set_target_properties(ma_strand_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_context_alloc_handler_test)
    set_target_properties(ma_context_alloc_handler_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_bind_handler_test)
    set_target_properties(ma_bind_handler_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_context_invoke_handler_test)
    set_target_properties(ma_context_invoke_handler_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_context_wrapped_handler_test)
    set_target_properties(ma_context_wrapped_handler_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_limited_int_test)
    set_target_properties(ma_limited_int_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_console_close_signal_test)
    set_target_properties(ma_console_close_signal_test PROPERTIES FOLDER "${project_group_tests}")

    add_subdirectory(tests/ma_intrusive_list_test)
    set_target_properties(ma_intrusive_list_test PROPERTIES FOLDER "${project_group_tests}")
endif()

# Examples of using of libraries
add_subdirectory(examples/ma_asio_performance_test_client)
set_target_properties(ma_asio_performance_test_client PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/ma_async_basics)
set_target_properties(ma_async_basics PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/ma_async_basics2)
set_target_properties(ma_async_basics2 PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/ma_echo_server)
set_target_properties(ma_echo_server PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/ma_echo_server_core)
set_target_properties(ma_echo_server_core PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/ma_nmea_client)
set_target_properties(ma_nmea_client PROPERTIES FOLDER "${project_group_examples}")

if(ma_qt)
    add_subdirectory(examples/ma_qt_echo_server)
    set_target_properties(ma_qt_echo_server PROPERTIES FOLDER "${project_group_examples}")
endif()

add_subdirectory(examples/asio_multicast_receiver)
set_target_properties(asio_multicast_receiver PROPERTIES FOLDER "${project_group_examples}")

add_subdirectory(examples/asio_multicast_sender)
set_target_properties(asio_multicast_sender PROPERTIES FOLDER "${project_group_examples}")
