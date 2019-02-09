#!/bin/sh

# Exit immediately if command fails
set -e

# Setup required version of Qt if needed
if [[ "${MA_QT}" = "ON" ]]; then
    if [[ "${MA_QT_MAJOR_VERSION}" -ne 4 ]]; then
        # Remove Qt 4 to eliminate conflicts with another version of Qt
        apk -q -f del qt-dev || true;
    fi
    if [[ "${MA_QT_MAJOR_VERSION}" -ne 5 ]]; then
        # Remove Qt 5 to eliminate conflicts with another version of Qt
        apk -q -f del qt5-qtbase-dev || true;
    fi
    # Install requested version of Qt
    if [[ "${MA_QT_MAJOR_VERSION}" -eq 4 ]]; then
        apk -f add --no-cache qt-dev;
    fi
    if [[ "${MA_QT_MAJOR_VERSION}" -eq 5 ]]; then
        apk -f add --no-cache qt5-qtbase-dev;
    fi
fi

# Generate Makefile project
if [[ "${STATIC_RUNTIME}" = "ON" ]]; then
    cmake -D CMAKE_USER_MAKE_RULES_OVERRIDE=${PROJECT_DIR}/cmake/static_c_runtime_overrides.cmake \
          -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${PROJECT_DIR}/cmake/static_cxx_runtime_overrides.cmake \
          -D CMAKE_SKIP_BUILD_RPATH=ON \
          -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
          -D Boost_USE_STATIC_LIBS=${BOOST_USE_STATIC_LIBS} \
          -D ma_qt=${MA_QT} -D ma_force_qt_major_version=${MA_QT_MAJOR_VERSION} \
          -D ma_coverage_build=${COVERAGE_BUILD} \
           $@ \
           ${PROJECT_DIR}
else
    cmake -D CMAKE_SKIP_BUILD_RPATH=ON \
          -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
          -D Boost_USE_STATIC_LIBS=${BOOST_USE_STATIC_LIBS} \
          -D ma_qt=${MA_QT} -D ma_force_qt_major_version=${MA_QT_MAJOR_VERSION} \
          -D ma_coverage_build=${COVERAGE_BUILD} \
           $@ \
           ${PROJECT_DIR}
fi

# Perform building of project
cmake --build . --config ${BUILD_TYPE}

# Prepare zero counters - baseline - for code coverage calculation
if [[ "${COVERAGE_BUILD}" = "ON" ]]; then
    lcov -z -d . &&\
    lcov -c -d . -i -o lcov-base.info;
fi

# Run tests
ctest --build-config ${BUILD_TYPE} --verbose

# Calculate difference to get code coverage statistic and generate HTML report
if [[ "${COVERAGE_BUILD}" = "ON" ]]; then
    lcov -c -d . -o lcov-test.info &&\
    lcov -a lcov-base.info -a lcov-test.info -o lcov.info &&\
    lcov -r lcov.info "$(pwd)/**/ui_*.h*" "$(pwd)/**/moc_*.c*" "/usr/*" "${PROJECT_DIR}/3rdparty/*" "${PROJECT_DIR}/examples/*" "${PROJECT_DIR}/tests/*" -o lcov.info &&\
    genhtml -o coverage lcov.info;
fi
