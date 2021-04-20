#
# Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM ubuntu:20.04 AS build

RUN apt-get update && \
    apt-get install -y --no-install-suggests --no-install-recommends \
      ca-certificates \
      curl \
      git \
      g++ \
      make \
      libstdc++-9-dev && \
    rm -rf /var/lib/apt/lists/*

ENV CMAKE_HOME="/opt/cmake"

ARG CMAKE_VERSION="3.20.1"

RUN mkdir -p "${CMAKE_HOME}" && \
    cmake_url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" && \
    echo "Downloading CMake ${CMAKE_VERSION} from ${cmake_url} to ${CMAKE_HOME}" && \
    curl -jksSL "${cmake_url}" | tar -xzf - -C "${CMAKE_HOME}" --strip-components 1

ENV PATH="${CMAKE_HOME}/bin:${PATH}" \
    BOOST_HOME="/usr/lib/boost"

ARG BOOST_VERSION="1.76.0"
ARG BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost"

RUN mkdir -p "${BOOST_HOME}" && \
    boost_archive_url="${BOOST_URL}/${BOOST_VERSION}/boost-${BOOST_VERSION}-x64-gcc$(gcc -dumpversion | sed -r 's/^([[:digit:]]+)(\..*)?$/\1/;t;d').tar.gz" && \
    echo "Downloading Boost from ${boost_archive_url} to ${BOOST_HOME}" && \
    curl --connect-timeout 300 --max-time 1800 --retry 10 --retry-delay 10 \
      -jksSL "${boost_archive_url}" \
      | tar -xz -C "${BOOST_HOME}" --strip-components 1

ARG MA_REVISION="master"

RUN source_dir="$(mktemp -d)" && \
    git clone "https://github.com/mabrarov/asio_samples.git" "${source_dir}" && \
    git -C "${source_dir}" checkout "${MA_REVISION}" && \
    build_dir="$(mktemp -d)" && \
    cmake \
      -D CMAKE_SKIP_BUILD_RPATH=ON \
      -D CMAKE_BUILD_TYPE=RELEASE \
      -D Boost_USE_STATIC_LIBS=ON \
      -D Boost_NO_SYSTEM_PATHS=ON \
      -D BOOST_INCLUDEDIR="${BOOST_HOME}/include" \
      -D BOOST_LIBRARYDIR="${BOOST_HOME}/lib" \
      -D MA_TESTS=OFF \
      -D MA_QT=OFF \
      -S "${source_dir}" \
      -B "${build_dir}" && \
    cmake --build "${build_dir}" --target ma_echo_server && \
    mkdir -p /opt/ma_echo_server && \
    mv -f "${build_dir}/examples/ma_echo_server/ma_echo_server" \
        /opt/ma_echo_server/ && \
    rm -rf "${build_dir}" && \
    rm -rf "${source_dir}"

FROM gcr.io/distroless/cc-debian10

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]

COPY --from=build ["/opt/ma_echo_server", "/opt/ma_echo_server"]
