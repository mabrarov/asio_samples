# Asio samples 

[![Release](https://img.shields.io/github/release/mabrarov/asio_samples.svg)](https://github.com/mabrarov/asio_samples/releases/latest) [![License](https://img.shields.io/badge/license-BSL%201.0-brightgreen.svg)](LICENSE)

Branch | Linux | Windows |  Coverage | Coverity Scan
-------|-------|---------|-----------|--------------
[master](https://github.com/mabrarov/asio_samples/tree/master) | [![Travis CI build status](https://travis-ci.org/mabrarov/asio_samples.svg?branch=master)](https://travis-ci.org/mabrarov/asio_samples) | [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/m3m15b3wxkyhqfj2/branch/master?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-samples) | [![Code coverage status](https://codecov.io/gh/mabrarov/asio_samples/branch/master/graph/badge.svg)](https://codecov.io/gh/mabrarov/asio_samples/branch/master) | [![Coverity Scan status](https://scan.coverity.com/projects/9191/badge.svg)](https://scan.coverity.com/projects/mabrarov-asio_samples)  
[develop](https://github.com/mabrarov/asio_samples/tree/develop) | [![Travis CI build status](https://travis-ci.org/mabrarov/asio_samples.svg?branch=develop)](https://travis-ci.org/mabrarov/asio_samples) | [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/m3m15b3wxkyhqfj2/branch/develop?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-samples) | [![Code coverage status](https://codecov.io/gh/mabrarov/asio_samples/branch/develop/graph/badge.svg)](https://codecov.io/gh/mabrarov/asio_samples/branch/develop) |
[feature/test_coverage](https://github.com/mabrarov/asio_samples/tree/feature/test_coverage) | [![Travis CI build status](https://travis-ci.org/mabrarov/asio_samples.svg?branch=feature%2Ftest_coverage)](https://travis-ci.org/mabrarov/asio_samples) | [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/m3m15b3wxkyhqfj2/branch/feature/test_coverage?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-samples) | [![Code coverage status](https://codecov.io/gh/mabrarov/asio_samples/branch/feature%2Ftest_coverage/graph/badge.svg)](https://codecov.io/gh/mabrarov/asio_samples/branch/feature%2Ftest_coverage) |

Examples (code samples) describing the construction of active objects on the top of [Boost.Asio](http://www.boost.org/doc/libs/release/doc/html/boost_asio.html). 

A code-based guide for client/server creation with usage of active object pattern by means of [Boost C++ Libraries](http://www.boost.org).

## Building

Below examples assume:

* Windows Command Prompt with Windows SDK environment set up (for example, `VS2015 x64 Native Tools Command Prompt` Windows main menu link for Visual Studio 2015 x64) is used on Windows.
* Current directory is `asio_sample_build_dir`.
* This repository is cloned into `asio_samples_home_dir` directory.
* ICU library is located at `icu_home_dir` directory.
* Header files of Boost C++ Libraries are located at `boost_headers_dir` directory.
* Binary files of Boost C++ Libraries are located at `boost_libs_dir` directory.
* Qt 5.x is located at `qt5_home_dir` directory.
* Qt 4.x is located at `qt4_home_dir` directory.
* Google Test libraries are located at `gtest_home_dir` directory.
* `project_build_configuration` is [configuration to build](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html), i.e. one of:

  * DEBUG
  * RELEASE
  * RELWITHDEBINFO
  * MINSIZEREL

Building of CMake project is usually performed in 2 phases:

1. Generation of project for particular build system.
1. Building using generated project and particular build system. 

### Generation of project for building 

Use `ma_build_tests` to exclude tests from build (tests are included by default):

```cmd
cmake -D ma_build_tests=OFF ...
```

CMake project uses:

* [FindBoost CMake module](http://www.cmake.org/cmake/help/v3.1/module/FindBoost.html?highlight=findboost)
* [cmake-qt CMake module](http://www.cmake.org/cmake/help/v3.1/manual/cmake-qt.7.html) (refer to [Qt cmake manual](http://doc.qt.io/qt-5/cmake-manual.html) also)
* [FindGTest CMake module](https://cmake.org/cmake/help/v3.1/module/FindGTest.html)
* FindICU CMake module (see `FindICU.cmake`)

To build with [static C/C++ runtime](http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F) it is required:

* Use `CMAKE_USER_MAKE_RULES_OVERRIDE` CMake parameter pointing to `static_c_runtime_overrides.cmake`
* Use `CMAKE_USER_MAKE_RULES_OVERRIDE_CXX` CMake parameter pointing to `static_cxx_runtime_overrides.cmake`
* Static build of Qt
* Use `ICU_ROOT` CMake parameter pointing to root of ICU library (static build, required for Qt).

**Note** that on Windows `cmake-qt` searches for some system libraries (OpenGL) therefore to work correctly CMake should be executed after Windows SDK environment was set up (even if `Visual Studio` generator is used).

Example of generation of Visual Studio 2013 project (static C/C++ runtime, static Boost and static Qt 5, x64):

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples_home_dir/cmake/static_c_runtime_overrides.cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples_home_dir/cmake/static_cxx_runtime_overrides.cmake ^
-D ICU_ROOT=icu_home_dir ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
-G "Visual Studio 12 2013 Win64" ^
asio_samples_home_dir
```

Example of generation of Visual Studio 2015 project (shared C/C++ runtime, static Boost and shared Qt 5, x64):

```cmd
cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
G "Visual Studio 14 2015 Win64" ^
asio_samples_home_dir
```

Example of generation of Visual Studio 2015 project (shared C/C++ runtime, shared Boost and shared Qt 5, x64):

```cmd
cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^ 
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=OFF ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
-G "Visual Studio 14 2015 Win64" ^
asio_samples_home_dir
```

`Boost_USE_STATIC_LIBS` is turned `OFF` by default according to [FindBoost documentation](http://www.cmake.org/cmake/help/v3.1/module/FindBoost.html?highlight=findboost),
so `-D Boost_USE_STATIC_LIBS=OFF` can be omitted.

Example of generation of Visual Studio 2013 project (static C/C++ runtime, static Boost and static Qt 5, x64) for usage with Intel C++ Compiler XE 2015:

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples_home_dir/cmake/static_c_runtime_overrides.cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples_home_dir/cmake/static_cxx_runtime_overrides.cmake ^
-D ICU_ROOT=icu_home_dir ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
-D CMAKE_C_COMPILER=icl ^
-D CMAKE_CXX_COMPILER=icl ^
-T "Intel C++ Compiler XE 15.0" ^
-G "Visual Studio 12 2013 Win64" ^
asio_samples_home_dir
```

Example of generation of Visual Studio 2015 project (shared C/C++ runtime, static Boost and shared Qt 5, x64) for usage with Intel C++ Compiler 2016:

```cmd
cmake ^
-D ICU_ROOT=icu_home_dir ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
-D CMAKE_C_COMPILER=icl ^
-D CMAKE_CXX_COMPILER=icl ^
-T "Intel C++ Compiler 16.0" ^
-G "Visual Studio 14 2015 Win64" ^
asio_samples_home_dir
```

Remarks: use [this fix](https://software.intel.com/en-us/articles/limits1120-error-identifier-builtin-nanf-is-undefined) when using Intel C++ Compiler 16.0 with Visual Studio 2015 Update 1.

Example of generation of Visual Studio 2015 project (static C/C++ runtime, static Boost and no Qt, x64) for usage with Intel Parallel Studio XE 2017:

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples_home_dir/cmake/static_c_runtime_overrides.cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples_home_dir/cmake/static_cxx_runtime_overrides.cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D ma_qt=OFF ^
-D CMAKE_C_COMPILER=icl ^
-D CMAKE_CXX_COMPILER=icl ^
-T "Intel C++ Compiler 17.0" ^
-G "Visual Studio 14 2015 Win64" ^
asio_samples_home_dir
```

Example of generation of Visual Studio 2010 project (shared C/C++ runtime, static Boost and shared Qt 4):

```cmd
cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D QT_QMAKE_EXECUTABLE=qt4_home_dir/bin/qmake.exe ^
-D GTEST_ROOT=gtest_home_dir ^
-G "Visual Studio 10 2010" ^
asio_samples_home_dir
```

Example of generation of Visual Studio 2008 project (static C/C++ runtime, static Boost and static Qt 4):

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples_home_dir/cmake/static_c_runtime_overrides.cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples_home_dir/cmake/static_cxx_runtime_overrides.cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D QT_QMAKE_EXECUTABLE=qt4_home_dir/bin/qmake.exe ^
-D GTEST_ROOT=gtest_home_dir ^
-G "Visual Studio 9 2008" ^
asio_samples_home_dir
```

Example of generation of NMake makefile project (shared C/C++ runtime, static Boost and shared Qt 5):

```cmd
cmake ^
-D BOOST_INCLUDEDIR=boost_headers_dir ^
-D BOOST_LIBRARYDIR=boost_libs_dir ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR=qt5_home_dir/lib/cmake/Qt5Core ^
-D Qt5Gui_DIR=qt5_home_dir/lib/cmake/Qt5Gui ^
-D Qt5Widgets_DIR=qt5_home_dir/lib/cmake/Qt5Widgets ^
-D GTEST_ROOT=gtest_home_dir ^
-G "NMake Makefiles" ^
-D CMAKE_BUILD_TYPE=project_build_configuration ^
asio_samples_home_dir
```

Note that if Google Test was built with CMake and MS Visual Studio then you have to "install" it somehow - just copying results of build (add "d" postfix into the name of debug artifacts) into Google Test root directory (or into "lib" sub-directory) would be enough for [FindGTest](https://cmake.org/cmake/help/v3.1/module/FindGTest.html).

There is a copy of Google Test shipped as part of CMake project (refer to `3rdparty/gtest` directory). It will be used in case [FindGTest](https://cmake.org/cmake/help/v3.1/module/FindGTest.html) fails to find Google Test (so `GTEST_ROOT` is optional). Note that `-D gtest_force_shared_crt=ON` command line parameter is required in case shared C/C++ runtime is planned to be used with Google Test.

### Building generated project

Building of generated project can be done using chosen (during generation of project) build system or using CMake.

CMake command to build generated project:

```cmd
cmake --build asio_sample_build_dir --config project_build_configuration
```
