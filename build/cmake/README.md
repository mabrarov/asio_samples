CMake project
============

Uses :

* [FindBoost CMake module](http://www.cmake.org/cmake/help/v3.1/module/FindBoost.html?highlight=findboost)
* [cmake-qt CMake module](http://www.cmake.org/cmake/help/v3.1/manual/cmake-qt.7.html) (refer to [Qt cmake manual](http://doc.qt.io/qt-5/cmake-manual.html) also)
* FindICU CMake module (see `FindICU.cmake`)

To build with [static C/C++ runtime](http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F) use:

* `CMAKE_USER_MAKE_RULES_OVERRIDE` CMake parameter pointing to `static_c_runtime_overrides.cmake`
* `CMAKE_USER_MAKE_RULES_OVERRIDE_CXX` CMake parameter pointing to `static_cxx_runtime_overrides.cmake`
* Static build of Qt
* `ICU_ROOT` CMake parameter pointing to root of ICU library (static build, required for Qt).

**Note** that on Windows `cmake-qt` searches for some system libraries (OpenGL) therefore to work correctly 
CMake should be executed after Windows SDK environment was set up (even if `Visual Studio` generator is used).

Example of generation of Visual Studio 2013 project (static C/C++ runtime, static Boost and static Qt 5, x64):

```
cmake -D BOOST_INCLUDEDIR=<Boost headers directory> -D BOOST_LIBRARYDIR=<Boost built libraries directory> 
      -D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples/build/cmake/static_c_runtime_overrides.cmake 
      -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples/build/cmake/static_cxx_runtime_overrides.cmake 
      -D ICU_ROOT=<ICU root directory> -D Qt5Widgets_DIR=<Qt directory>/qtbase/lib/cmake/Qt5Widgets 
      -G "Visual Studio 12 2013 Win64" asio_samples/build/cmake
```

Example of generation of Visual Studio 2008 project (static C/C++ runtime, static Boost and static Qt 4):

```
cmake -D BOOST_INCLUDEDIR=<Boost headers directory> -D BOOST_LIBRARYDIR=<Boost built libraries directory> 
      -D CMAKE_USER_MAKE_RULES_OVERRIDE=asio_samples/build/cmake/static_c_runtime_overrides.cmake 
      -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=asio_samples/build/cmake/static_cxx_runtime_overrides.cmake 
      -D QT_QMAKE_EXECUTABLE=<Qt directory>/bin/qmake.exe
      -G "Visual Studio 9 2008" asio_samples/build/cmake
```

Example of generation of NMake makefile project (shared C/C++ runtime, static Boost and shared Qt 5):

```
cmake -D BOOST_INCLUDEDIR=<Boost headers directory> -D BOOST_LIBRARYDIR=<Boost built libraries directory> 
      -D Boost_USE_STATIC_LIBS=ON -D Qt5Widgets_DIR=<Qt directory>/qtbase/lib/cmake/Qt5Widgets 
      -G "NMake Makefiles" -D CMAKE_BUILD_TYPE=Release asio_samples/build/cmake
```
