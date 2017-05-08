#!/bin/sh

# Exit immediately if command fails
set -e

# Generate Makefile project
cmake -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -D Boost_USE_STATIC_LIBS=${BOOST_USE_STATIC_LIBS} \
      -D ma_qt=${MA_QT} -D ma_force_qt_major_version=${MA_QT_MAJOR_VERSION} \
      -D ma_coverage_build=${COVERAGE_BUILD} \
       $@ \
       /project

# Perform building of project
cmake --build . --config ${BUILD_TYPE}

if [[ "${COVERAGE_BUILD}" = "ON" ]]; then
    lcov -z -d . && lcov -c -d . -i -o lcov-base.info;
fi

# Run tests
ctest --build-config ${BUILD_TYPE} --verbose

if [[ "${COVERAGE_BUILD}" = "ON" ]]; then
    lcov -c -d . -o lcov-test.info &&\
    lcov -a lcov-base.info -a lcov-test.info -o lcov.info &&\
    lcov -r lcov.info 'ui_*.h*' 'moc_*.c*' '/usr/*' '3rdparty/*' 'examples/*' 'tests/*' -o lcov.info &&\
    genhtml -o coverage lcov.info;
fi
