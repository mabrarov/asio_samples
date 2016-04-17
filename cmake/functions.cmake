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
#   base_dir - directory being considered as a base for sub-directories which need to be found.
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
