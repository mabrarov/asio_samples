CMake project
============

Uses :

* [FindBoost CMake module](http://www.cmake.org/cmake/help/v3.1/module/FindBoost.html?highlight=findboost)
* [cmake-qt CMake module](http://www.cmake.org/cmake/help/v3.1/manual/cmake-qt.7.html) (refer to [Qt cmake manual](http://doc.qt.io/qt-5/cmake-manual.html) also)
* FindICU CMake module (see `FindICU.cmake`)

To build with [static C/C++ run-time](http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime) use:

* `CMAKE_USER_MAKE_RULES_OVERRIDE` CMake parameter poinying to `static_c_runtime_overrides.cmake`
* `CMAKE_USER_MAKE_RULES_OVERRIDE_CXX` CMake parameter poinying to `static_cxx_runtime_overrides.cmake`
* Static build of Qt
* `ICU_ROOT` CMake parameter poinying to root of ICU library (static build, required for Qt).

**Note** that on Windows `cmake-qt` searches for some system libraries (OpenGL) so to work correctly 
cmake should be executed after Windows SDK environment was set up (even if `Visual Studio` generator is used).
