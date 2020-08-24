#
# Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine:3.12.0 AS build

ARG MA_REVISION="master"
ARG BOOST_VERSION="1.74.0"
ARG BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost"

RUN apk add --no-cache \
      libstdc++ \
      linux-headers \
      g++ \
      make \
      cmake \
      git \
      curl \
      tar \
      gzip

ENV SOURCE_DIR="/tmp/asio_samples/src" \
    BUILD_DIR="/tmp/asio_samples/build" \
    DEPENDENCIES_DIR="/tmp/asio_samples/dependency"

RUN mkdir -p "${DEPENDENCIES_DIR}" && \
    boost_archive_url="${BOOST_URL}/${BOOST_VERSION}/boost-${BOOST_VERSION}-x64-gcc$(gcc -dumpversion | sed -r 's/^([[:digit:]]+)(\..*)?$/\1/;t;d')-musl-static-runtime.tar.gz" && \
    echo "Downloading Boost from ${boost_archive_url}" && \
    curl --connect-timeout 300 \
      --max-time 1800 \
      --retry 10 \
      --retry-delay 10 \
      -jksSL \
      "${boost_archive_url}" | tar -xz -C "${DEPENDENCIES_DIR}"

RUN mkdir -p "${SOURCE_DIR}" && \
    git clone https://github.com/mabrarov/asio_samples.git "${SOURCE_DIR}" && \
    cd "${SOURCE_DIR}" && \
    git checkout "${MA_REVISION}"

RUN mkdir -p "${BUILD_DIR}" && \
    cd "${BUILD_DIR}" && \
    boost_dir="${DEPENDENCIES_DIR}/boost-${BOOST_VERSION}-x64-gcc$(gcc -dumpversion | sed -r 's/^([[:digit:]]+)(\..*)?$/\1/;t;d')-musl" && \
    cmake \
      -D CMAKE_SKIP_BUILD_RPATH=ON \
      -D CMAKE_BUILD_TYPE=RELEASE \
      -D CMAKE_USER_MAKE_RULES_OVERRIDE="${SOURCE_DIR}/cmake/static_c_runtime_overrides.cmake" \
      -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="${SOURCE_DIR}/cmake/static_cxx_runtime_overrides.cmake" \
      -D Boost_USE_STATIC_LIBS=ON \
      -D Boost_NO_SYSTEM_PATHS=ON \
      -D BOOST_INCLUDEDIR="${boost_dir}/include" \
      -D BOOST_LIBRARYDIR="${boost_dir}/lib" \
      -D MA_TESTS=OFF \
      -D MA_QT=OFF \
      "${SOURCE_DIR}" && \
    cmake --build . --target ma_echo_server

FROM scratch

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

COPY --from=build /tmp/asio_samples/build/examples/ma_echo_server/ma_echo_server /opt/ma_echo_server/ma_echo_server

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]
