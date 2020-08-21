#!/bin/bash

#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# Exit immediately if command fails
set -e

if [[ "${MA_QT}" = "ON" ]] && [[ "${MA_QT_MAJOR_VERSION}" -eq 4 ]]; then
  echo "Qt 4 is not supported by Ubuntu Linux anymore"
  exit 1
fi

# Generate Makefile project
if [[ "${STATIC_RUNTIME}" = "ON" ]]; then
  cmake \
    -D CMAKE_USER_MAKE_RULES_OVERRIDE="${PROJECT_DIR}/cmake/static_c_runtime_overrides.cmake" \
    -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="${PROJECT_DIR}/cmake/static_cxx_runtime_overrides.cmake" \
    -D CMAKE_SKIP_BUILD_RPATH=ON \
    -D CMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -D Boost_USE_STATIC_LIBS="${BOOST_USE_STATIC_LIBS}" \
    -D MA_QT="${MA_QT}" -D MA_QT_MAJOR_VERSION="${MA_QT_MAJOR_VERSION}" \
    -D MA_COVERAGE="${MA_COVERAGE}" \
    "${@}" \
    "${PROJECT_DIR}"
else
  cmake \
    -D CMAKE_SKIP_BUILD_RPATH=ON \
    -D CMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -D Boost_USE_STATIC_LIBS="${BOOST_USE_STATIC_LIBS}" \
    -D MA_QT="${MA_QT}" -D MA_QT_MAJOR_VERSION="${MA_QT_MAJOR_VERSION}" \
    -D MA_COVERAGE="${MA_COVERAGE}" \
    "${@}" \
    "${PROJECT_DIR}"
fi

# Perform building of project
cmake --build . --config "${BUILD_TYPE}"

# Prepare zero counters - baseline - for code coverage calculation
if [[ "${MA_COVERAGE}" = "ON" ]]; then
  lcov -z -d .
  lcov -c -d . -i -o lcov-base.info
fi

# Run tests
ctest --build-config "${BUILD_TYPE}" --verbose

# Calculate difference to get code coverage statistic and generate HTML report
if [[ "${MA_COVERAGE}" = "ON" ]]; then
  lcov -c -d . -o lcov-test.info
  lcov -a lcov-base.info -a lcov-test.info -o lcov.info
  lcov -r lcov.info \
    "$(pwd)/**/ui_*.h*" \
    "$(pwd)/**/moc_*.c*" \
    "/usr/*" \
    "${PROJECT_DIR}/3rdparty/*" \
    "${PROJECT_DIR}/examples/*" \
    "${PROJECT_DIR}/tests/*" \
    -o lcov.info
  genhtml -o coverage lcov.info
fi
