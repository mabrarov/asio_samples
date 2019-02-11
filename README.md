# Asio samples

[![Release](https://img.shields.io/github/release/mabrarov/asio_samples.svg)](https://github.com/mabrarov/asio_samples/releases/latest) [![License](https://img.shields.io/badge/license-BSL%201.0-brightgreen.svg)](https://github.com/mabrarov/asio_samples/tree/master/LICENSE) 

Branch | Linux | Windows | Coverage | Coverity Scan
-------|-------|---------|----------|--------------
[master](https://github.com/mabrarov/asio_samples/tree/master) | [![Travis CI build status](https://travis-ci.org/mabrarov/asio_samples.svg?branch=master)](https://travis-ci.org/mabrarov/asio_samples) | [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/m3m15b3wxkyhqfj2/branch/master?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-samples) | [![Linux code coverage status](https://codecov.io/gh/mabrarov/asio_samples/branch/master/graph/badge.svg)](https://codecov.io/gh/mabrarov/asio_samples/branch/master) | [![Coverity Scan status](https://scan.coverity.com/projects/9191/badge.svg)](https://scan.coverity.com/projects/mabrarov-asio_samples)  
[develop](https://github.com/mabrarov/asio_samples/tree/develop) | [![Travis CI build status](https://travis-ci.org/mabrarov/asio_samples.svg?branch=develop)](https://travis-ci.org/mabrarov/asio_samples) | [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/m3m15b3wxkyhqfj2/branch/develop?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-samples) | [![Linux code coverage status](https://codecov.io/gh/mabrarov/asio_samples/branch/develop/graph/badge.svg)](https://codecov.io/gh/mabrarov/asio_samples/branch/develop) | |

Extended examples for [Boost.Asio](http://www.boost.org/doc/libs/release/doc/html/boost_asio.html).

## Building with Docker container

Refer to [docker/builder/README.md](docker/builder/README.md) for instruction on how to build Linux version with existing Docker images.

## Build manual

### Prerequisites

* C++ toolchain (one of):
  * MinGW with MSYS make

    MinGW-W64 4.9+

  * Visual Studio (Visual C++)

    2008, 2010, 2012, 2013, 2015 are tested versions

  * GCC with make

    4.6+

  * Clang with make

    3.6+  

* [CMake](https://cmake.org/)

  3.0+, consider using the latest version of CMake because it supports recent versions of libraries

* [Boost](https://www.boost.org)

  1.47+, the latest tested version is 1.69

* [Google Test](https://github.com/google/googletest) 

  Optional, copy of 1.7.0 version is shipped with this project and is used if no other instance is found

* [Qt](https://www.qt.io)

  Optional, 4.0+, both 4.x and 5.x are supported with default preference to 5.x

* [ICU](http://site.icu-project.org/home)

  Optional, required if static Qt 5.x is used, version should match Qt requirements / version which was used to build static Qt

### Assumptions

* `%...%` syntax is used for Windows Command Prompt and `${...}` syntax is used for *nix shell
* Windows Command Prompt with configured Windows SDK environment is used on Windows
* Directory with generated project files and with built binaries is specified in `asio_samples_build_dir` environment variable
* This repository is cloned into directory specified in `asio_samples_home_dir` environment variable
* ICU is located at directory specified by `icu_home_dir` environment variable
* Header files of Boost are located at directory specified by `boost_headers_dir` environment variable
* Binary files of Boost are located at directory specified by `boost_libs_dir` environment variable
* Qt 5.x is located at directory specified by `qt5_home_dir` environment variable
* Qt 4.x is located at directory specified by `qt4_home_dir` environment variable
* Google Test is located at directory specified by `gtest_home_dir` environment variable
* [build type](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html) is specified in `build_type` environment variable and is one of:
  * Debug
  * Release
  * RelWithDebInfo
  * MinSizeRel
* `cmake_generator` environment variable is [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) (underlying build system), like:
  * `Visual Studio 14 2015 Win64` - Visual Studio 2015, x64
  * `Visual Studio 12 2013 Win64` - Visual Studio 2013, x64
  * `Visual Studio 10 2010` - Visual Studio 2010, x86
  * `Visual Studio 9 2008` - Visual Studio 2008, x86
  * `NMake Makefiles` - NMake
  * `MinGW Makefiles` - MinGW makefiles
  * `Unix Makefiles` - *nix makefiles

Building of CMake project is usually performed in 2 steps:

1. [Generation of project for build system](#generation-of-project-for-build-system)
1. [Building generated project with build system](#building-generated-project-with-build-system)

### Generation of project for build system

Assuming current directory is `asio_samples_build_dir`.

Windows Command Prompt:

```cmd
cmake ... -G "%cmake_generator%" "%asio_samples_home_dir%"
```

*nix shell

```bash
cmake ... -G "${cmake_generator}" "${asio_samples_home_dir}"
```

where `...` are optional parameters which are described below.

Use `ma_build_tests` CMake variable to exclude tests from build (tests are included by default):

```
cmake -D ma_build_tests=OFF ...
```

CMake project uses CMake find modules, so most of parameters comes from these CMake modules:

* [FindBoost CMake module](http://www.cmake.org/cmake/help/latest/module/FindBoost.html?highlight=findboost)

  CMake variables which can be specified in command line to locate and configure Boost:

  * `BOOST_INCLUDEDIR` - Boost headers directory

    Windows Command Prompt:

    ```
    -D BOOST_INCLUDEDIR="%boost_headers_dir%"
    ```

    *nix shell

    ```
    -D BOOST_INCLUDEDIR="${boost_headers_dir}"
    ```

  * `BOOST_LIBRARYDIR` - Boost libraries directory

    Windows Command Prompt:

    ```
    -D BOOST_LIBRARYDIR="%boost_libs_dir%"
    ```

    *nix shell

    ```
    -D BOOST_LIBRARYDIR="${boost_libs_dir}"
    ```

  * `Boost_USE_STATIC_LIBS` - force usage of static Boost, possible values are `ON` (use static libraries) and `OFF` (linkage type depends on platform)
  * `Boost_NO_SYSTEM_PATHS` - do not search for Boost in default system locations

* [cmake-qt CMake module](http://www.cmake.org/cmake/help/latest/manual/cmake-qt.7.html) (refer to [Qt cmake manual](http://doc.qt.io/qt-5/cmake-manual.html) also)

  CMake variables which can be specified in command line to locate Qt 5.x:

  * `Qt5Core_DIR`
  
    Windows Command Prompt:

    ```
    -D Qt5Core_DIR="%qt5_home_dir%/lib/cmake/Qt5Core"
    ```

    *nix shell

    ```
    -D Qt5Core_DIR="${qt5_home_dir}/lib/cmake/Qt5Core"
    ```

  * `Qt5Gui_DIR`

    Windows Command Prompt:

    ```
    -D Qt5Gui_DIR="%qt5_home_dir%/lib/cmake/Qt5Gui"
    ```

    *nix shell

    ```
    -D Qt5Gui_DIR="${qt5_home_dir}/lib/cmake/Qt5Gui"
    ```

  * `Qt5Widgets_DIR`

    Windows Command Prompt:

    ```
    -D Qt5Widgets_DIR="%qt5_home_dir%/lib/cmake/Qt5Widgets"
    ```

    *nix shell

    ```
    -D Qt5Widgets_DIR="${qt5_home_dir}/lib/cmake/Qt5Widgets"
    ```

  CMake variables which can be specified in command line to locate Qt 4.x:

  * `QT_QMAKE_EXECUTABLE` - path to `qmake` executable

    Windows Command Prompt:

    ```
    -D QT_QMAKE_EXECUTABLE="%qt4_home_dir%/bin/qmake.exe"
    ```

    *nix shell

    ```
    -D QT_QMAKE_EXECUTABLE="${qt4_home_dir}/bin/qmake.exe"
    ```

  `ma_qt` CMake variable can be used to avoid usage of Qt and to skip examples using Qt.
  Possible values are:
  
  * `ON` (default) - search for (require) and use Qt
  * `OFF` - do not search for Qt and skip building the parts which use Qt

* [FindGTest CMake module](https://cmake.org/cmake/help/latest/module/FindGTest.html)

  CMake variables which can be specified in command line to locate Google Test:

  * `GTEST_ROOT` - Google Test home directory

    Windows Command Prompt:

    ```
    -D GTEST_ROOT="%gtest_home_dir%"
    ```

    *nix shell

    ```
    -D GTEST_ROOT="${gtest_home_dir}"
    ```

  Note that if Google Test was built with CMake and MS Visual Studio then you have to "install" it somehow - just copying results of build (add "d" postfix into the name of debug artifacts) into Google Test root directory (or into "lib" sub-directory) would be enough for [FindGTest](https://cmake.org/cmake/help/latest/module/FindGTest.html).

  There is a copy of Google Test shipped as part of CMake project (refer to [3rdparty/gtest](3rdparty/gtest) directory). 
  It will be used in case [FindGTest](https://cmake.org/cmake/help/latest/module/FindGTest.html) fails to find Google Test (so `GTEST_ROOT` is optional).
  Note that `-D gtest_force_shared_crt=ON` command line parameter is required in case shared C/C++ runtime is planned to be used with Google Test on Windows.

* [FindICU CMake module](cmake/ma_qt5_core_support/FindICU.cmake)

  Used only if static Qt 5.x is identified.

  CMake variables which can be specified in command line to locate ICU:

  * `ICU_ROOT` - ICU home directory
  
    Windows Command Prompt:

    ```
    -D ICU_ROOT="%icu_home_dir%"
    ```

    *nix shell

    ```
    -D ICU_ROOT="${icu_home_dir}"
    ```

To build with [static C/C++ runtime](https://gitlab.kitware.com/cmake/community/wikis/FAQ#how-can-i-build-my-msvc-application-with-a-static-runtime):

* Use `CMAKE_USER_MAKE_RULES_OVERRIDE` CMake variable pointing to [cmake/static_c_runtime_overrides.cmake](cmake/static_c_runtime_overrides.cmake)

  Windows Command Prompt:
  
  ```
  -D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_samples_home_dir%/cmake/static_c_runtime_overrides.cmake"
  ```

  *nix shell

  ```
  -D CMAKE_USER_MAKE_RULES_OVERRIDE="${asio_samples_home_dir}/cmake/static_c_runtime_overrides.cmake"
  ```

* Use `CMAKE_USER_MAKE_RULES_OVERRIDE_CXX` CMake variable pointing to [cmake/static_cxx_runtime_overrides.cmake](cmake/static_cxx_runtime_overrides.cmake)

  Windows Command Prompt:

  ```
  -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_samples_home_dir%/cmake/static_cxx_runtime_overrides.cmake"
  ```

  *nix shell

  ```
  -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="${asio_samples_home_dir}/cmake/static_cxx_runtime_overrides.cmake"
  ```

* Static build of Qt
* `ICU_ROOT` CMake variable pointing to ICU home directory (static build, required for Qt).

**Note** that on Windows `cmake-qt` searches for some system libraries (OpenGL) therefore to work correctly CMake should be executed after Windows SDK environment was set up (even if `Visual Studio` generator is used).

The CMake project searches for Qt 5.x first and if Qt 5.x is not found then it searches for Qt 4.x.
This can be changed with `ma_force_qt_major_version` CMake variable which can be specified in command line. 

Possible values are:

* 4 - search for Qt 4.x only

  ```
  -D ma_force_qt_major_version=4
  ```

* 5 - search for Qt 5.x only

  ```
  -D ma_force_qt_major_version=5
  ```

Example of generation of Visual Studio 2015 project with:

* static C/C++ runtime
* static Boost
* static Qt 5.x
* internal copy of Google Test
* x64 build

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_samples_home_dir%/cmake/static_c_runtime_overrides.cmake" ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_samples_home_dir%/cmake/static_cxx_runtime_overrides.cmake" ^
-D ICU_ROOT="%icu_home_dir%" ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR="%qt5_home_dir%/lib/cmake/Qt5Core" ^
-D Qt5Gui_DIR="%qt5_home_dir%/lib/cmake/Qt5Gui" ^
-D Qt5Widgets_DIR="%qt5_home_dir%/lib/cmake/Qt5Widgets" ^
-G "Visual Studio 14 2015 Win64" ^
"%asio_samples_home_dir%"
```

Example of generation of makefiles on *nix with:

* shared C/C++ runtime
* static Boost 
* shared Qt 5.x

```bash
cmake \
-D Boost_USE_STATIC_LIBS=ON \
-D ma_force_qt_major_version=5 \
-D CMAKE_BUILD_TYPE=${build_type} \
-G "Unix Makefiles" \
"${asio_samples_home_dir}"
```

Example of generation of Visual Studio 2015 project with:

* shared C/C++ runtime
* static Boost (which uses shared C/C++ runtime)
* shared Qt 5.x
* internal copy of Google Test
* x64 build

```cmd
cmake ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D gtest_force_shared_crt=ON ^
-D Qt5Core_DIR="%qt5_home_dir%/lib/cmake/Qt5Core" ^
-D Qt5Gui_DIR="%qt5_home_dir%/lib/cmake/Qt5Gui" ^
-D Qt5Widgets_DIR="%qt5_home_dir%/lib/cmake/Qt5Widgets" ^
-G "Visual Studio 14 2015 Win64" ^
"%asio_samples_home_dir%"
```

Example of generation of Visual Studio 2013 project with:

* static C/C++ runtime
* static Boost
* static Qt 5
* external Google Test
* x64 build
* Intel C++ Compiler XE 2015

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_samples_home_dir%/cmake/static_c_runtime_overrides.cmake" ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_samples_home_dir%/cmake/static_cxx_runtime_overrides.cmake" ^
-D ICU_ROOT="%icu_home_dir%" ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D Qt5Core_DIR="%qt5_home_dir%/lib/cmake/Qt5Core" ^
-D Qt5Gui_DIR="%qt5_home_dir%/lib/cmake/Qt5Gui" ^
-D Qt5Widgets_DIR="%qt5_home_dir%/lib/cmake/Qt5Widgets" ^
-D GTEST_ROOT="%gtest_home_dir%" ^
-D CMAKE_C_COMPILER=icl ^
-D CMAKE_CXX_COMPILER=icl ^
-T "Intel C++ Compiler XE 15.0" ^
-G "Visual Studio 12 2013 Win64" ^
"%asio_samples_home_dir%"
```

Example of generation of Visual Studio 2015 project with:

* shared C/C++ runtime
* shared Boost
* shared Qt 5
* x64 build
* Intel C++ Compiler 2016

```cmd
cmake ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=OFF ^
-D gtest_force_shared_crt=ON ^
-D Qt5Core_DIR="%qt5_home_dir%/lib/cmake/Qt5Core" ^
-D Qt5Gui_DIR="%qt5_home_dir%/lib/cmake/Qt5Gui" ^
-D Qt5Widgets_DIR="%qt5_home_dir%/lib/cmake/Qt5Widgets" ^
-D CMAKE_C_COMPILER=icl ^
-D CMAKE_CXX_COMPILER=icl ^
-T "Intel C++ Compiler 16.0" ^
-G "Visual Studio 14 2015 Win64" ^
"%asio_samples_home_dir%"
```

Example of generation of Visual Studio 2010 project with:

* shared C/C++ runtime
* static Boost (which uses shared C/C++ runtime)
* shared Qt 4.x
* x86 build

```cmd
cmake ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D gtest_force_shared_crt=ON ^
-D QT_QMAKE_EXECUTABLE="%qt4_home_dir%/bin/qmake.exe" ^
-G "Visual Studio 10 2010" ^
"%asio_samples_home_dir%"
```

Example of generation of Visual Studio 2008 project with:

* static C/C++ runtime
* static Boost
* static Qt 4.x
* x86 build

```cmd
cmake ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_samples_home_dir%/cmake/static_c_runtime_overrides.cmake" ^
-D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_samples_home_dir%/cmake/static_cxx_runtime_overrides.cmake" ^
-D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
-D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
-D Boost_NO_SYSTEM_PATHS=ON ^
-D Boost_USE_STATIC_LIBS=ON ^
-D QT_QMAKE_EXECUTABLE="%qt4_home_dir%/bin/qmake.exe" ^
-G "Visual Studio 9 2008" ^
"%asio_samples_home_dir%"
```

### Building generated project with build system

Building of generated project can be done using chosen (during generation of project) build system or using CMake.

Assuming current directory is `asio_samples_build_dir`.

CMake command to build generated project (Windows Command Prompt):

```cmd
cmake --build . --config %build_type%
```

CMake command to build generated project (*nix shell):

```bash
cmake --build . --config ${build_type}
```

### Running tests

Assuming current directory is `asio_samples_build_dir`.

[CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) command to run built tests (Windows Command Prompt):

```cmd
ctest --build-config %build_type%
```

CTest command to run built tests (*nix shell):

```bash
ctest --build-config ${build_type}
```
