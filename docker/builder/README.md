# Using [Docker](https://docs.docker.com/) for building Asio samples

## Common

_&lt;directory_with_project&gt;_ is assumed to be a directory with project (root). 
_&lt;directory_with_results_of_build&gt;_ is assumed to be existing directory where the results of build (CMake generated project) will be placed.

Environment variables to control build process:

| Variable name  | Meaning of variable | Possible values | Default value  | Comments  |
|---|---|---|---|---|
| BUILD_TYPE | Type of project CMake generates. Is passed to CMake "as is". Is passed to CMake as [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) parameter. | One of: `DEBUG`,  `RELEASE`,  `RELWITHDEBINFO`, `MINSIZEREL`. Refer to [CMake documentation](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) for details. | `RELEASE` | Use `DEBUG` if calculating code coverage. |
| STATIC_RUNTIME | Enforces static linkage with C/C++ runtime. | One of: `ON`, `OFF`. | `OFF` |   |
| BOOST_USE_STATIC_LIBS | Enforces static linkage with [Boost C++ Libraries](http://www.boost.org/). Is passed to CMake command line during generation of project "as is" under `Boost_USE_STATIC_LIBS` parameter. | One of: `ON` (static linkage), `OFF` (linkage type depends on platform). | `ON` | Refer to documentation of [FindBoost](https://cmake.org/cmake/help/v3.0/module/FindBoost.html) CMake module for details about `Boost_USE_STATIC_LIBS`. |
| MA_QT | Enables or disables usage of Qt and so enables / disables parts of the project which depend on Qt. | One of: `ON`, `OFF`. | `ON` |   |
| MA_QT_MAJOR_VERSION | Enforces usage of Qt 4.x or Qt 5.x. | One of: `4` (for usage of Qt 4.x), `5` (for usage of Qt 5.x). | `5` | Is ignored if `MA_QT == OFF`. |
| COVERAGE_BUILD | Turns on calculation of code coverage and generation of coverage report (HTML) under `coverage` subdirectory of directory with results of build. | One of: `ON` (with code coverage), `OFF` (without code coverage). | `OFF` | It's recommended to calculate code coverage with debug build, i.e. with `BUILD_TYPE == DEBUG`. |

All the Docker Run command line parameters specified after name of image are passed to CMake during generation of project as is, i.e. this command:

```
docker run ... <image-name> -D MY_PARAM=my_value
```

leads to passing `-D MY_PARAM=my_value` to CMake command line during generation of project.

## Alpine Linux

Use `abrarov/asio-samples-builder-alpine` as _&lt;image-name&gt;_.

## Ubuntu

Use `abrarov/asio-samples-builder-ubuntu` as _&lt;image-name&gt;_.

## CentOS

Use `abrarov/asio-samples-builder-centos` as _&lt;image-name&gt;_.

## Build steps

Use below command to build with Qt 5.x and to calculate code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build release version with Qt 5.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build with Qt 4.x and to calculate code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build release version with Qt 4.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```
