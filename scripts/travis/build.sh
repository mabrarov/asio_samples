#!/bin/bash

set -e

export CODECOV_FLAG="${TRAVIS_OS_NAME}__$(uname -r | sed -r 's/[[:space:]]|[\\\.\/:]/_/g')__${CXX_COMPILER_FAMILY}_$(${CXX_COMPILER} -dumpversion)__boost_${BOOST_VERSION}__qt_${QT_MAJOR_VERSION}"
export CODECOV_FLAG="${CODECOV_FLAG//[.-]/_}"

echo "Preparing build dir at ${BUILD_HOME}"
rm -rf "${BUILD_HOME}"
mkdir -p "${BUILD_HOME}"

export COVERAGE_BUILD=0
if [[ "${COVERAGE_BUILD_CANDIDATE}" != 0 ]]; then
  export COVERAGE_BUILD=1
fi

cd "${BUILD_HOME}"
if [[ "${COVERITY_SCAN_BRANCH}" != 1 ]]; then
  export CMAKE_GENERATOR_COMMAND="cmake -D CMAKE_C_COMPILER=\"${C_COMPILER}\" -D CMAKE_CXX_COMPILER=\"${CXX_COMPILER}\" -D CMAKE_BUILD_TYPE=\"${BUILD_TYPE}\""
  if [[ "${BOOST_FROM_PACKAGE}" != 0 ]]; then
    echo "Building with Boost ${BOOST_VERSION} installed from package"
  else
    echo "Building with Boost ${BOOST_VERSION} located at ${PREBUILT_BOOST_HOME}"
    export CMAKE_GENERATOR_COMMAND="${CMAKE_GENERATOR_COMMAND} -D CMAKE_SKIP_BUILD_RPATH=ON -D Boost_NO_SYSTEM_PATHS=ON -D BOOST_INCLUDEDIR=\"${PREBUILT_BOOST_HOME}/include\" -D BOOST_LIBRARYDIR=\"${PREBUILT_BOOST_HOME}/lib\""
  fi
  export CMAKE_GENERATOR_COMMAND="${CMAKE_GENERATOR_COMMAND} -D MA_QT_MAJOR_VERSION=\"${QT_MAJOR_VERSION}\""
  if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
    if [[ "${QT_MAJOR_VERSION}" == 5 ]]; then
      export Qt5_DIR="/usr/local/opt/qt5/lib/cmake"
      export CMAKE_GENERATOR_COMMAND="${CMAKE_GENERATOR_COMMAND} -D Qt5Core_DIR=\"${Qt5_DIR}/Qt5Core\" -D Qt5Gui_DIR=\"${Qt5_DIR}/Qt5Gui\" -D Qt5Widgets_DIR=\"${Qt5_DIR}/Qt5Widgets\""
    fi
  fi
  export CMAKE_GENERATOR_COMMAND="${CMAKE_GENERATOR_COMMAND} -D MA_COVERAGE=\"${COVERAGE_BUILD}\" \"${TRAVIS_BUILD_DIR}\""
  echo "CMake project generation command: ${CMAKE_GENERATOR_COMMAND}"
  eval ${CMAKE_GENERATOR_COMMAND}
  cmake --build "${BUILD_HOME}" --config "${BUILD_TYPE}"
fi

if [[ "${COVERAGE_BUILD}" != 0 ]]; then
  echo "Preparing code coverage counters at ${BUILD_HOME}/lcov-base.info"
  lcov -z -d "${BUILD_HOME}"
  lcov -c -d "${BUILD_HOME}" -i -o lcov-base.info --rc lcov_branch_coverage=1
fi

ctest --build-config "${BUILD_TYPE}" --verbose

if [[ "${COVERAGE_BUILD}" != 0 ]]; then
  echo "Caclulating coverage at ${BUILD_HOME}/lcov-test.info"
  lcov -c -d "${BUILD_HOME}" -o lcov-test.info --rc lcov_branch_coverage=1
  echo "Caclulating coverage delta at ${BUILD_HOME}/lcov.info"
  lcov -a lcov-base.info -a lcov-test.info -o lcov.info --rc lcov_branch_coverage=1
  echo "Excluding 3rd party code from coverage data located at ${BUILD_HOME}/lcov.info"
  lcov -r lcov.info "ui_*.h*" "moc_*.c*" "/usr/*" "3rdparty/*" "examples/*" "tests/*" "${DEPENDENCIES_HOME}/*" -o lcov.info --rc lcov_branch_coverage=1
fi

if [[ "${COVERAGE_BUILD}" != 0 ]]; then
  echo "Sending ${BUILD_HOME}/lcov.info coverage data to Codecov" &&
  bash <(curl \
    --connect-timeout "${CURL_CONNECT_TIMEOUT}" \
    --max-time "${CURL_MAX_TIME}" \
    --retry "${CURL_RETRY}" \
    --retry-delay "${CURL_RETRY_DELAY}" \
    -s https://codecov.io/bash) \
    -Z \
    -f "${BUILD_HOME}/lcov.info" \
    -R "${TRAVIS_BUILD_DIR}" \
    -X gcov
    -F "${CODECOV_FLAG}"
fi

cd "${TRAVIS_BUILD_DIR}"
