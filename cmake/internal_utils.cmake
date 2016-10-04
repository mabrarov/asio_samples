#
# Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)

# Builds list of sub-directories (relative paths).
# Parameters:
#   files    - files or directories to scan (list).
#   base_dir - directory being considered as a base if file name is relative.
#   results  - name of variable to store list of sub-directories.
function(ma_list_subdirs files base_dir results)
    get_filename_component(cmake_base_dir "${base_dir}" ABSOLUTE)
    file(TO_CMAKE_PATH "${cmake_base_dir}" cmake_base_dir)

    set("${cmake_base_dir}" case_normalized_base_dir)
    if(CMAKE_HOST_WIN32)
        string(TOUPPER "${cmake_base_dir}" case_normalized_base_dir)
    endif()

    set(subdirs )
    set(subdir_found FALSE)
    foreach(file IN LISTS files)
        get_filename_component(file_path "${file}" PATH)        
        if(NOT IS_ABSOLUTE "${file_path}")
            file(TO_CMAKE_PATH "${file_path}" file_path)
            set("${cmake_base_dir}/${file_path}" file_path)
        endif()
        get_filename_component(file_path "${file_path}" ABSOLUTE)
        file(TO_CMAKE_PATH "${file_path}" file_path)

        set("${file_path}" case_normalized_file_path)
        if(CMAKE_HOST_WIN32)
            string(TOUPPER "${file_path}" case_normalized_file_path)
        endif()        

        string(FIND "${case_normalized_file_path}" "${case_normalized_base_dir}" start_pos)
        if(${start_pos} EQUAL 0)
            file(RELATIVE_PATH subdir "${cmake_base_dir}" "${file_path}")
            list(APPEND subdirs "${subdir}")
            set(subdir_found TRUE)
        endif()
    endforeach()        
    if(${subdir_found})
        list(REMOVE_DUPLICATES subdirs)
    endif()

    set(${results} "${subdirs}" PARENT_SCOPE)
endfunction()

# Filters list of files (or directories) by specified location (directory). 
# Only files located directly in specified location are returned.
# Parameters:
#   files      - files or directories to filter (list).
#   base_dir   - directory being considered as a base for files (directories) 
#                which name is not absolute.
#   filter_dir - directory which is used to filter souce list. 
#                Only files located directly in this directory are returned.
#   results    - name of variable to store filtered list.
function(ma_filter_files files base_dir filter_dir results)
    get_filename_component(cmake_base_dir "${base_dir}" ABSOLUTE)
    get_filename_component(cmake_filter_dir "${filter_dir}" ABSOLUTE)
    file(TO_CMAKE_PATH "${cmake_base_dir}"   cmake_base_dir)
    file(TO_CMAKE_PATH "${cmake_filter_dir}" cmake_filter_dir)
    if(CMAKE_HOST_WIN32)
        string(TOUPPER "${cmake_filter_dir}" cmake_filter_dir)
    endif()

    set(filtered_files )
    foreach(file IN LISTS files)
        get_filename_component(file_path "${file}" PATH)
        if(NOT IS_ABSOLUTE "${file_path}")
            file(TO_CMAKE_PATH "${file_path}" file_path)
            set("${cmake_base_dir}/${file_path}" file_path)
        endif()
        get_filename_component(file_path "${file_path}" ABSOLUTE)
        file(TO_CMAKE_PATH "${file_path}" file_path)
        if(CMAKE_HOST_WIN32)
            string(TOUPPER "${file_path}" file_path)
        endif()        
        if("${file_path}" STREQUAL "${cmake_filter_dir}")
            list(APPEND filtered_files "${file}")
        endif()
    endforeach()

    set(${results} "${filtered_files}" PARENT_SCOPE)
endfunction()

# Builds source groups based on relative location of files.
# Files located out of base directroy are not included into any source group.
# Parameters:
#   base_group_name - base source group name for base directory (refer to base_dir parameter).
#   base_dir        - base directory being considered as a base for files and correlating 
#                     with base source group name. 
#   files           - files (list) to associate with source groups built according to relative 
#                     (comparing with base_dir) paths.
function(ma_dir_source_group base_group_name base_dir files)
    ma_list_subdirs("${files}" "${base_dir}" subdirs)
    foreach(subdir IN LISTS subdirs)
        string(REPLACE "/" "\\" subdir_group_name "${base_group_name}/${subdir}")
        ma_filter_files("${files}" "${CMAKE_CURRENT_BINARY_DIR}" "${base_dir}/${subdir}" subdir_files)
        source_group("${subdir_group_name}" FILES ${subdir_files})
    endforeach()
endfunction()

# Changes existing (default) compiler options.
# Parameters:
#   result - name of list to store compile options.
function(change_default_compile_options orignal_compile_options result)
    set(compile_options ${orignal_compile_options})
    # Turn on more strict warning mode
    if(MSVC)
        if(compile_options MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4" compile_options "${compile_options}")
        else()
            set(compile_options "${compile_options} /W4")
        endif()
    endif()
    set(${result} "${compile_options}" PARENT_SCOPE)
endfunction()

# Builds list of additional internal compiler options.
# Parameters:
#   result - name of list to store compile options.
function(config_private_compile_options result)
    set(compile_options )
    # Turn on more strict warning mode
    if(MSVC)
        list(APPEND compile_options "/W4")
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND compile_options "-Wall" "-Wextra" "-pedantic" "-Wunused" "-Wno-long-long")
    endif()
    set(${result} "${compile_options}" PARENT_SCOPE)
endfunction()

# Builds list of additional transitive compiler options.
# Parameters:
#   result - name of list to store compile options.
function(config_public_compile_options result)
    set(compile_options )
    # Turn on thread support for GCC
    if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        if(MINGW)
            list(APPEND compile_options "-mthreads")
        else()
            list(APPEND compile_options "-pthread")
        endif()
    endif()
    # Turn on support of C++11 if it's available
    if(CMAKE_COMPILER_IS_GNUCXX)
        if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7"))
            list(APPEND compile_options "-std=c++11")
        elseif(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.3"))
            list(APPEND compile_options "-std=c++0x")
        endif()
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        list(APPEND compile_options "-std=c++11")
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
        if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14"))
            list(APPEND compile_options "/Qstd=c++11")
        elseif(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12"))
            list(APPEND compile_options "/Qstd=c++0x")
        endif()
    endif()
    set(${result} "${compile_options}" PARENT_SCOPE)
endfunction()

# Builds list of additional internal compiler definitions.
# Parameters:
#   result - name of list to store compile definitions.
function(config_private_compile_definitions result)
    set(compile_definitions )
    set(${result} "${compile_definitions}" PARENT_SCOPE)
endfunction()

# Builds list of additional transitive compiler definitions.
# Parameters:
#   result - name of list to store compile definitions.
function(config_public_compile_definitions result)
    set(compile_definitions )
    # Additional preprocessor definitions for Windows target
    if(WIN32)
        list(APPEND compile_definitions
            WIN32
            WIN32_LEAN_AND_MEAN
            WINVER=0x0501
            _WIN32_WINNT=0x0501
            _WIN32_WINDOWS=0x0501
            _WIN32_IE=0x0600
            _UNICODE
            UNICODE
            _WINSOCK_DEPRECATED_NO_WARNINGS)
    endif()
    set(${result} "${compile_definitions}" PARENT_SCOPE)
endfunction()
