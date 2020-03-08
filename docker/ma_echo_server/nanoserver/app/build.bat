@echo off

rem
rem Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
rem
rem Distributed under the Boost Software License, Version 1.0. (See accompanying
rem file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
rem

set exit_code=0

call "%MSVC_BUILD_DIR%\%MSVC_CMD_BOOTSTRAP%"%MSVC_CMD_BOOTSTRAP_OPTIONS%
set exit_code=%errorlevel%
if %exit_code% neq 0 goto exit

set PATH=%CMAKE_HOME%\bin;%PATH%

cmake ^
  -D CMAKE_USER_MAKE_RULES_OVERRIDE="%SOURCE_DIR%/cmake/static_c_runtime_overrides.cmake" ^
  -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%SOURCE_DIR%/cmake/static_cxx_runtime_overrides.cmake" ^
  -D BOOST_INCLUDEDIR="%BOOST_INCLUDE_DIR%" ^
  -D BOOST_LIBRARYDIR="%BOOST_LIBRARY_DIR%" ^
  -D Boost_NO_SYSTEM_PATHS=ON ^
  -D Boost_USE_STATIC_LIBS=ON ^
  -D MA_TESTS=OFF ^
  -D MA_QT=OFF ^
  -G "%CMAKE_GENERATOR%" ^
  -A "%CMAKE_GENERATOR_PLATFORM%" ^
  "%SOURCE_DIR%"
set exit_code=%errorlevel%
if %exit_code% neq 0 goto exit

cmake --build . --config Release --target ma_echo_server
set exit_code=%errorlevel%
if %exit_code% neq 0 goto exit

:exit
exit /B %exit_code%
