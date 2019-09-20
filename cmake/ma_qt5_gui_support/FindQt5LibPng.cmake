# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)

# Prefix for all Qt5::LibPng relative public variables
if(NOT DEFINED QT5_LIB_PNG_PUBLIC_VAR_NS)
    set(QT5_LIB_PNG_PUBLIC_VAR_NS "Qt5LibPng")
endif()

# Prefix for all Qt5::LibPng relative internal variables
if(NOT DEFINED QT5_LIB_PNG_PRIVATE_VAR_NS)
    set(QT5_LIB_PNG_PRIVATE_VAR_NS "_${QT5_LIB_PNG_PUBLIC_VAR_NS}")
endif()

set(${QT5_LIB_PNG_PUBLIC_VAR_NS}_FOUND FALSE)
set(${QT5_LIB_PNG_PUBLIC_VAR_NS}_LIBRARIES )

find_package(Qt5Gui)

if(Qt5Gui_FOUND)
    # Extract directory of Qt Core library
    get_target_property(${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_LOCATION
        Qt5::Gui
        LOCATION)
    get_filename_component(${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_DIR 
        "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_LOCATION}" 
        PATH)

    set(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIBRARIES )
    set(${QT5_LIB_PNG_PRIVATE_VAR_NS}_BASE_NAME "qtlibpng")

    find_library(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_RELEASE 
        "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_BASE_NAME}"
        HINTS "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_DIR}"
        DOC "Release library of Qt5::LibPng")

    find_library(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_DEBUG
        "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_BASE_NAME}d"
        HINTS "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_DIR}"
        DOC "Debug library of Qt5::LibPng")

    if(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_RELEASE OR ${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_DEBUG)
        set(${QT5_LIB_PNG_PUBLIC_VAR_NS}_FOUND TRUE)
    endif()

    if(${QT5_LIB_PNG_PUBLIC_VAR_NS}_FOUND)   
        # Determine linkage type of imported Qt5::LibPng (via linkage type of Qt5::Gui library)
        get_property(${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_TARGET_TYPE 
            TARGET Qt5::Gui PROPERTY TYPE)
        set(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LINKAGE_TYPE "SHARED")
        if(${${QT5_LIB_PNG_PRIVATE_VAR_NS}_QT_GUI_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
            set(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LINKAGE_TYPE "STATIC")
        endif()

        add_library(Qt5::LibPng 
            ${${QT5_LIB_PNG_PRIVATE_VAR_NS}_LINKAGE_TYPE} IMPORTED)

        if(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_RELEASE)
            set_property(TARGET Qt5::LibPng APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(Qt5::LibPng PROPERTIES IMPORTED_LOCATION_RELEASE 
                "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_RELEASE}")
            list(APPEND ${QT5_LIB_PNG_PUBLIC_VAR_NS}_LIBRARIES 
                optimized ${${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_RELEASE})
        endif()

        if(${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_DEBUG)
            set_property(TARGET Qt5::LibPng APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(Qt5::LibPng PROPERTIES IMPORTED_LOCATION_DEBUG 
                "${${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_DEBUG}")
            list(APPEND ${QT5_LIB_PNG_PUBLIC_VAR_NS}_LIBRARIES 
                debug ${${QT5_LIB_PNG_PRIVATE_VAR_NS}_LIB_DEBUG})
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
if(${QT5_LIB_PNG_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${QT5_LIB_PNG_PUBLIC_VAR_NS}_FIND_QUIETLY)
    find_package_handle_standard_args(${QT5_LIB_PNG_PUBLIC_VAR_NS}
        FOUND_VAR ${QT5_LIB_PNG_PUBLIC_VAR_NS}_FOUND
        REQUIRED_VARS ${QT5_LIB_PNG_PUBLIC_VAR_NS}_LIBRARIES)
else()
    find_package_handle_standard_args(${QT5_LIB_PNG_PUBLIC_VAR_NS}
        FOUND_VAR ${QT5_LIB_PNG_PUBLIC_VAR_NS}_FOUND
        REQUIRED_VARS ${QT5_LIB_PNG_PUBLIC_VAR_NS}_LIBRARIES
        FAIL_MESSAGE "Qt5::LibPng not found")
endif()
