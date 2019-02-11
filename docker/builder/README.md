# Using [Docker](https://docs.docker.com/) for building Asio samples

## Common

`${directory_with_project}` is assumed to be a directory with project (root). 
`${directory_with_results_of_build}` is assumed to be existing directory where the results of build (CMake generated project) will be placed.

Environment variables to control build process:

| Variable name  | Meaning of variable | Possible values | Default value  | Comments  |
|---|---|---|---|---|
| BUILD_TYPE | Type of project CMake generates. Is passed to CMake "as is". Is passed to CMake as [CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) parameter. | One of: `Debug`,  `Release`,  `RelWithDebInfo`, `MinSizeRel`. Refer to [CMake documentation](https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html) for details. | `Release` | Use `Debug` if calculating code coverage. |
| STATIC_RUNTIME | Enforces static linkage with C/C++ runtime. | One of: `ON`, `OFF`. | `OFF` |   |
| BOOST_USE_STATIC_LIBS | Enforces static linkage with [Boost C++ Libraries](http://www.boost.org/). Is passed to CMake command line during generation of project "as is" under `Boost_USE_STATIC_LIBS` parameter. | One of: `ON` (static linkage), `OFF` (linkage type depends on platform). | `ON` | Refer to documentation of [FindBoost](https://cmake.org/cmake/help/v3.0/module/FindBoost.html) CMake module for details about `Boost_USE_STATIC_LIBS`. |
| MA_QT | Enables or disables usage of Qt and so enables / disables parts of the project which depend on Qt. | One of: `ON`, `OFF`. | `ON` |   |
| MA_QT_MAJOR_VERSION | Enforces usage of Qt 4.x or Qt 5.x. | One of: `4` (for usage of Qt 4.x), `5` (for usage of Qt 5.x). | `5` | Is ignored if `MA_QT == OFF`. |
| COVERAGE_BUILD | Turns on calculation of code coverage and generation of coverage report (HTML) under `coverage` subdirectory of directory with results of build. | One of: `ON` (with code coverage), `OFF` (without code coverage). | `OFF` | It's recommended to calculate code coverage with debug build, i.e. with `BUILD_TYPE == Debug`. |

All the Docker Run command line parameters specified after name of image are passed to CMake during generation of project as is, i.e. this command:

```bash
docker run ... ${image_name} -D MY_PARAM=my_value
```

leads to passing `-D MY_PARAM=my_value` to CMake command line during generation of project.

## Alpine Linux

Use `abrarov/asio-samples-builder-alpine` as `${image_name}`

## Ubuntu

Use `abrarov/asio-samples-builder-ubuntu` as `${image_name}`.

## CentOS

Use `abrarov/asio-samples-builder-centos` as `${image_name}`.

## Build steps

Use below command to build with Qt 5.x and to calculate code coverage:

```bash
docker run --rm \
-e MA_QT_MAJOR_VERSION=5 \
-e BUILD_TYPE=Debug \
-e COVERAGE_BUILD=ON \
-v ${directory_with_project}:/project:ro \
-v ${directory_with_results_of_build}:/build \
${image_name}
```

Use below command to build release version with Qt 5.x:

```bash
docker run --rm \
-e MA_QT_MAJOR_VERSION=5 \
-v ${directory_with_project}:/project:ro \
-v ${directory_with_results_of_build}:/build \
${image_name}
```

Use below command to build with Qt 4.x and to calculate code coverage:

```bash
docker run --rm \
-e MA_QT_MAJOR_VERSION=4 \
-e BUILD_TYPE=Debug \
-e COVERAGE_BUILD=ON \
-v ${directory_with_project}:/project:ro \
-v ${directory_with_results_of_build}:/build \
${image_name}
```

Use below command to build release version with Qt 4.x:

```bash
docker run --rm \
-e MA_QT_MAJOR_VERSION=4 \
-v ${directory_with_project}:/project:ro \
-v ${directory_with_results_of_build}:/build \
${image_name}
```

## Building with static C/C++ runtime

### Prerequisites

1. Get prebuilt ([Alpine](https://bintray.com/mabrarov/generic/download_file?file_path=boost%2F1.69.0%2Fboost-1.69.0-alpine39-x64-gcc820-static-runtime.tar.gz), [CentOS](https://bintray.com/mabrarov/generic/download_file?file_path=boost%2F1.69.0%2Fboost-1.69.0-centos7-x64-gcc485-static-runtime.tar.gz)) or build Boost C++ Libraries with static C/C++ runtime
1. Get prebuilt or build Qt with static C/C++ runtime (as well as all dependencies of Qt like ICU) or use `MA_QT` environment variable to skip usage of Qt and to not build parts which require Qt

### Steps

Assuming built Boost C++ Libraries are installed into `${boost_install_root}`

```bash
docker run --rm \
-e STATIC_RUNTIME=ON \
-e MA_QT=OFF \
-v ${directory_with_project}:/project:ro \
-v ${directory_with_results_of_build}:/build \
-v ${boost_install_root}:/boost:ro \
${image_name} \
-D Boost_NO_SYSTEM_PATHS=ON \
-D BOOST_INCLUDEDIR=/boost/include \
-D BOOST_LIBRARYDIR=/boost/lib
```