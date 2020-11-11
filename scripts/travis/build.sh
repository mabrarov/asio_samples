#!/bin/bash

#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

set -e

# shellcheck source=travis_retry.sh
source "${TRAVIS_BUILD_DIR}/scripts/travis/travis_retry.sh"

c_compiler_family="$(echo "${C_COMPILER:-gcc}" | sed -r 's/^([^\-]+)(\-.*)?$/\1/;t;d')"
c_compiler_major_version="$(${C_COMPILER:-gcc} --version | sed -r "s/^(.*[[:space:]]+)?${c_compiler_family}[[:space:]]+[^[:digit:]]*([[:digit:]]+)(\..*)?$/\2/;t;d")"

codecov_flag="${TRAVIS_OS_NAME}__$(uname -r | sed -r 's/[[:space:]]|[\\\.\/:]/_/g')__${c_compiler_family}_${c_compiler_major_version}__boost_${BOOST_VERSION}__qt_${QT_MAJOR_VERSION}"
codecov_flag="${codecov_flag//[.-]/_}"

echo "Preparing build dir at ${BUILD_HOME}"
rm -rf "${BUILD_HOME}"
mkdir -p "${BUILD_HOME}"
cd "${BUILD_HOME}"

generate_cmd="cmake -D CMAKE_C_COMPILER=$(printf "%q" "${C_COMPILER}")"
generate_cmd="${generate_cmd} -D CMAKE_CXX_COMPILER=$(printf "%q" "${CXX_COMPILER}")"
generate_cmd="${generate_cmd} -D CMAKE_BUILD_TYPE=$(printf "%q" "${BUILD_TYPE}")"
if [[ -n "${BOOST_HOME+x}" ]]; then
  echo "Building with Boost ${BOOST_VERSION} located at ${BOOST_HOME}"
  generate_cmd="${generate_cmd} -D CMAKE_SKIP_BUILD_RPATH=ON -D Boost_NO_SYSTEM_PATHS=ON"
  generate_cmd="${generate_cmd} -D BOOST_INCLUDEDIR=$(printf "%q" "${BOOST_HOME}/include")"
  generate_cmd="${generate_cmd} -D BOOST_LIBRARYDIR=$(printf "%q" "${BOOST_HOME}/lib")"
else
  echo "Building with Boost ${BOOST_VERSION} located at system paths"
fi
generate_cmd="${generate_cmd} -D MA_QT_MAJOR_VERSION=$(printf "%q" "${QT_MAJOR_VERSION}")"
if [[ "${TRAVIS_OS_NAME}" = "osx" ]]; then
  if [[ "${QT_MAJOR_VERSION}" -eq 5 ]]; then
    qt5_dir="/usr/local/opt/qt5/lib/cmake"
    generate_cmd="${generate_cmd} -D Qt5Core_DIR=$(printf "%q" "${qt5_dir}/Qt5Core")"
    generate_cmd="${generate_cmd} -D Qt5Gui_DIR=$(printf "%q" "${qt5_dir}/Qt5Gui")"
    generate_cmd="${generate_cmd} -D Qt5Widgets_DIR=$(printf "%q" "${qt5_dir}/Qt5Widgets")"
  fi
fi
generate_cmd="${generate_cmd} -D MA_COVERAGE=$(printf "%q" "${COVERAGE_BUILD}")"
generate_cmd="${generate_cmd} $(printf "%q" "${TRAVIS_BUILD_DIR}")"
echo "CMake project generation command: ${generate_cmd}"
eval "${generate_cmd}"
cmake --build "${BUILD_HOME}" --config "${BUILD_TYPE}"

if [[ "${COVERAGE_BUILD}" -ne 0 ]]; then
  echo "Preparing code coverage counters at ${BUILD_HOME}/lcov-base.info"
  lcov -z -d "${BUILD_HOME}"
  lcov -c -d "${BUILD_HOME}" -i -o lcov-base.info --rc lcov_branch_coverage=1
fi

ctest --build-config "${BUILD_TYPE}" --verbose

if [[ "${COVERAGE_BUILD}" -ne 0 ]]; then
  echo "Calculating coverage at ${BUILD_HOME}/lcov-test.info"
  lcov -c -d "${BUILD_HOME}" -o lcov-test.info --rc lcov_branch_coverage=1
  echo "Calculating coverage delta at ${BUILD_HOME}/lcov.info"
  lcov -a lcov-base.info -a lcov-test.info -o lcov.info --rc lcov_branch_coverage=1
  echo "Excluding 3rd party code from coverage data located at ${BUILD_HOME}/lcov.info"
  lcov -r lcov.info \
    "ui_*.h*" \
    "moc_*.c*" \
    "/usr/*" \
    "3rdparty/*" \
    "examples/*" \
    "tests/*" \
    "${DEPENDENCIES_HOME}/*" \
    -o lcov.info --rc lcov_branch_coverage=1
fi

cd "${TRAVIS_BUILD_DIR}"

if [[ "${COVERAGE_BUILD}" -ne 0 ]]; then
  echo "Sending ${BUILD_HOME}/lcov.info coverage data to Codecov"
  travis_retry codecov \
    --required \
    --token "${CODECOV_TOKEN}" \
    --file "${BUILD_HOME}/lcov.info" \
    --flags "${codecov_flag}" \
    -X gcov \
    --root "${TRAVIS_BUILD_DIR}"
fi
