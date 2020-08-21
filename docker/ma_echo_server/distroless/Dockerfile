#
# Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM ubuntu:20.04 AS build

ARG MA_REVISION="master"
ARG CMAKE_VERSION="3.18.2"
ARG BOOST_VERSION="1.74.0"
ARG BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost"

ENV TZ="Europe/Moscow"

RUN ln -snf "/usr/share/zoneinfo/${TZ}" /etc/localtime && \
    echo "${TZ}" > /etc/timezone && \
    apt-get update && \
    apt-get install -y \
      curl \
      git \
      g++ \
      make \
      libstdc++-9-dev

ENV PATH="/opt/cmake/bin:${PATH}" \
    SOURCE_DIR="/tmp/asio_samples/src" \
    BUILD_DIR="/tmp/asio_samples/build" \
    DEPENDENCIES_DIR="/tmp/asio_samples/dependency"

RUN mkdir -p /opt && \
    cmake_url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" && \
    echo "Downloading CMake ${CMAKE_VERSION} from ${cmake_url}" && \
    curl -jksSL "${cmake_url}" | tar -xzf - -C /opt && \
    mv -f "/opt/cmake-${CMAKE_VERSION}-Linux-x86_64" /opt/cmake

RUN mkdir -p "${DEPENDENCIES_DIR}" && \
    boost_archive_url="${BOOST_URL}/${BOOST_VERSION}/boost-${BOOST_VERSION}-x64-gcc$(gcc -dumpversion | sed -r 's/^([[:digit:]]+)(\..*)?$/\1/;t;d').tar.gz" && \
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
    boost_dir="${DEPENDENCIES_DIR}/boost-${BOOST_VERSION}-x64-gcc$(gcc -dumpversion | sed -r 's/^([[:digit:]]+)(\..*)?$/\1/;t;d')" && \
    cmake \
      -D CMAKE_SKIP_BUILD_RPATH=ON \
      -D CMAKE_BUILD_TYPE=RELEASE \
      -D Boost_USE_STATIC_LIBS=ON \
      -D Boost_NO_SYSTEM_PATHS=ON \
      -D BOOST_INCLUDEDIR="${boost_dir}/include" \
      -D BOOST_LIBRARYDIR="${boost_dir}/lib" \
      -D MA_TESTS=OFF \
      -D MA_QT=OFF \
      "${SOURCE_DIR}" && \
    cmake --build . --target ma_echo_server

FROM gcr.io/distroless/cc-debian10

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

COPY --from=build /tmp/asio_samples/build/examples/ma_echo_server/ma_echo_server /opt/ma_echo_server/ma_echo_server

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]
