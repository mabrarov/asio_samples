#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(ma_qt_echo_server)

set(project_base_dir "${PROJECT_SOURCE_DIR}")
set(cxx_headers_dir  "${project_base_dir}/include")
set(cxx_sources_dir  "${project_base_dir}/src")

set(cxx_compile_definitions )
set(cxx_headers )
set(cxx_sources )
set(cxx_libraries )
set(boost_components )

set(uic_forms )
set(qrc_resources )
set(moc_headers )
set(native_resources )
set(native_resource_headers )

set(moc_headers )
set(uic_generated_headers )
set(qrc_generated_sources )

# Select used Boost libraries
find_package(Boost REQUIRED)
list(APPEND boost_components
    exception
    system
    date_time
    regex)
if(NOT(${Boost_VERSION} LESS 104700))
    list(APPEND boost_components
        chrono)
    set(ma_boost_has_chrono TRUE)
else()
    set(ma_boost_has_chrono FALSE)
endif()

# Attach used Boost libraries
find_package(Boost REQUIRED COMPONENTS ${boost_components})

# Enforce linkage with shared build of Boost C++ Libraries in case
# there is no request for linkage with static build
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()

# Determine major version of Qt to use (Qt 5 is predeferred)
if(NOT DEFINED ma_qt_echo_server_use_qt5)
    set(ma_qt_echo_server_use_qt5 TRUE)
    find_package(Qt5Widgets QUIET)
    if(NOT Qt5Widgets_FOUND)
        set(ma_qt_echo_server_use_qt5 FALSE)
    endif()
endif()

# Find required Qt libraries
if(ma_qt_echo_server_use_qt5)
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
        Qt4::QtCor
        Qt4::QtGui)
endif()

list(APPEND uic_forms
    "${cxx_sources_dir}/mainform.ui")

list(APPEND moc_headers
    "${cxx_headers_dir}/ma/echo/server/qt/mainform.h"
    "${cxx_headers_dir}/ma/echo/server/qt/service.h"
    "${cxx_headers_dir}/ma/echo/server/qt/serviceforwardsignal.h"
    "${cxx_headers_dir}/ma/echo/server/qt/serviceservantsignal.h")

list(APPEND cxx_headers
    "${cxx_headers_dir}/ma/echo/server/qt/custommetatypes.h"
    "${cxx_headers_dir}/ma/echo/server/qt/execution_config.h"
    "${cxx_headers_dir}/ma/echo/server/qt/execution_config_fwd.h"
    "${cxx_headers_dir}/ma/echo/server/qt/serviceforwardsignal_fwd.h"
    "${cxx_headers_dir}/ma/echo/server/qt/serviceservantsignal_fwd.h"
    "${cxx_headers_dir}/ma/echo/server/qt/servicestate.h"
    "${cxx_headers_dir}/ma/echo/server/qt/service_fwd.h"
    "${cxx_headers_dir}/ma/echo/server/qt/meta_type_register_error.h"
    "${cxx_headers_dir}/ma/echo/server/qt/signal_connect_error.h"
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

set(ma_qt_static FALSE)
if(ma_qt_echo_server_use_qt5)
    # Determine linkage type of Qt libraries (via linkage type of Qt Core library)
    get_property(Qt5_Core_TYPE TARGET Qt5::Core PROPERTY TYPE)
    if(${Qt5_Core_TYPE} STREQUAL "STATIC_LIBRARY")
        set(ma_qt_static TRUE)
    endif()

    # Statically linked Qt requires:
    #     All libraries used by Qt internally to be linked explicitly to the target.
    #     Used Qt plugins to be linked explicitly to the target.
    if(ma_qt_static)
        # Qt CMake files use IMPORTED_LINK_DEPENDENT_LIBRARIES instead of INTERFACE_LINK_LIBRARIES for OpenGL dependencies.
        # So we need to add those transitive (in case of static linkage) dependencies explicitly.
        list(APPEND qt_libraries
            ${Qt5Gui_EGL_LIBRARIES}
            ${Qt5Gui_OPENGL_LIBRARIES})

        # Add required platform integration plugins
        if(WIN32)
            list(APPEND qt_libraries
                Qt5::QWindowsIntegrationPlugin)
        else()
            # todo:
            # add support for the rest of platforms supported by Qt
            # by linking with platform specific plugin
            # (add platfrom integration plugin to cxx_libraries)
        endif()

        # Some Qt libraries have no cmake files so wee need to search them ourselves
        # Refer to custom CMake modules:
        #     FindICU.cmake
        #     FindQt5PlatformSupport.cmake
        #     FindQt5HarfBuzzNG.cmake
        #     FindQt5FreeType.cmake
        #     FindQt5PCRE.cmake

        find_package(Qt5PlatformSupport REQUIRED)
        list(APPEND qt_libraries
            Qt5::PlatformSupport)

        find_package(ICU REQUIRED COMPONENTS i18n uc data)
        list(APPEND qt_libraries
            ${ICU_LIBRARIES})

        # Starting from Qt 5.4.0 HarfBuzz-NG library is included (at least Windows version of Qt 5.4.0)
        if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.4.0"))
            find_package(Qt5HarfBuzzNG QUIET)
            if(Qt5HarfBuzzNG_FOUND)
                list(APPEND qt_libraries
                    Qt5::HarfBuzzNG)
            endif()
        endif()

        # Starting from Qt 5.5.0 FreeType library is used by QtGui
        if(WIN32 AND NOT(${Qt5Gui_VERSION} VERSION_LESS "5.5.0"))
            find_package(Qt5FreeType QUIET)
            if(Qt5FreeType_FOUND)
                list(APPEND qt_libraries
                    Qt5::FreeType)
            endif()
        endif()

        # Starting from Qt 5.5.0 PCRE library is used by QtCore
        if(WIN32 AND NOT(${Qt5Core_VERSION} VERSION_LESS "5.5.0"))
            find_package(Qt5PCRE QUIET)
            if(Qt5PCRE_FOUND)
                list(APPEND qt_libraries
                    Qt5::PCRE)
            endif()
        endif()
    endif()
else()
    # Statically linked Qt requires:
    #     All libraries used by Qt internally to be linked explicitly to the target.
    #     Used Qt plugins to be linked explicitly to the target.
    if(QT_IS_STATIC)
        # Nothing to add as no Qt 4 plugins are used.
        set(ma_qt_static TRUE)
    endif()
endif()

list(APPEND cxx_libraries
    "${Boost_LIBRARIES}"
    "${qt_libraries}"
    ma_compat
    ma_helpers
    ma_thread_group
    ma_echo_server_core)

# Required Windows libraries
if(WIN32)
    list(APPEND cxx_libraries
        "ws2_32"
        "mswsock"
        "comdlg32"
        "uuid"
        "ole32"
        "oleaut32"
        "advapi32"
        "imm32"
        "winmm"
        "winspool")
endif()

# Boost.Chrono requires rt library on *nix
if(UNIX AND ma_boost_has_chrono)
    list(APPEND cxx_libraries
        "rt")
endif()

# Required for Qt UIC
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Wrap Qt generated sources.
if(ma_qt_echo_server_use_qt5)
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
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    ${cxx_compile_definitions})
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${cxx_headers_dir})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${cxx_libraries})

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
