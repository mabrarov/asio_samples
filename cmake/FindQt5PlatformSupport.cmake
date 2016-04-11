# 
# Copyright (c) 2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)

# Prefix for all Qt5::PlatformSupport relative public variables
if(NOT DEFINED QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS)
    set(QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS "Qt5PlatformSupport")
endif()

# Prefix for all Qt5::PlatformSupport relative internal variables
if(NOT DEFINED QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS)
    set(QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS "_${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}")
endif()

set(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FOUND FALSE)
set(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_LIBRARIES )

find_package(Qt5Core)

if(Qt5Core_FOUND)
    # Extract directory of Qt Core library
    get_target_property(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_LOCATION 
        Qt5::Core
        LOCATION)
    get_filename_component(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_DIR 
        "${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_LOCATION}" 
        PATH)

    set(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIBRARIES )

    find_library(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_RELEASE 
        "Qt5PlatformSupport"
        HINTS "${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_DIR}"
        DOC "Release library of Qt5::PlatformSupport")

    find_library(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_DEBUG 
        "Qt5PlatformSupportd" 
        HINTS "${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_DIR}"
        DOC "Debug library of Qt5::PlatformSupport")

    if(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_RELEASE OR ${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_DEBUG)
        set(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FOUND TRUE)
    endif()

    if(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FOUND)   
        # Determine linkage type of imported Qt5::PlatformSupport (via linkage type of Qt5::Core library)
        get_property(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_TARGET_TYPE 
            TARGET Qt5::Core PROPERTY TYPE)
        set(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LINKAGE_TYPE "SHARED")
        if(${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_QT_CORE_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
            set(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LINKAGE_TYPE "STATIC")
        endif()

        add_library(Qt5::PlatformSupport 
            ${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LINKAGE_TYPE} IMPORTED)

        if(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_RELEASE)
            set_property(TARGET Qt5::PlatformSupport APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(Qt5::PlatformSupport PROPERTIES IMPORTED_LOCATION_RELEASE 
                "${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_RELEASE}")
            list(APPEND ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_LIBRARIES 
                optimized ${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_RELEASE})
        endif()

        if(${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_DEBUG)
            set_property(TARGET Qt5::PlatformSupport APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(Qt5::PlatformSupport PROPERTIES IMPORTED_LOCATION_DEBUG 
                "${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_DEBUG}")
            list(APPEND ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_LIBRARIES 
                debug ${${QT5_PLATFORM_SUPPORT_PRIVATE_VAR_NS}_LIB_DEBUG})
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
if(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FIND_QUIETLY)
    find_package_handle_standard_args(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}
        FOUND_VAR ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FOUND
        REQUIRED_VARS ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_LIBRARIES)
else()
    find_package_handle_standard_args(${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}
        FOUND_VAR ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_FOUND
        REQUIRED_VARS ${QT5_PLATFORM_SUPPORT_PUBLIC_VAR_NS}_LIBRARIES
        FAIL_MESSAGE "Qt5::PlatformSupport not found")
endif()
