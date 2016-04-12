#
# Copyright (c) 2015-2016 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

cmake_minimum_required(VERSION 2.8.11)
project(test)

#add_subdirectory(ma_async_connect_test)
add_subdirectory(ma_handler_storage_test)
add_subdirectory(ma_lockable_wrapper_test)
#add_subdirectory(ma_shared_ptr_factory_test)
#add_subdirectory(ma_sp_singleton_test)
if(WIN32)
    #add_subdirectory(ma_windows_console_signal_test)
endif()
