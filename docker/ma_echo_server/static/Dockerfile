#
# Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine:3.13.2 AS build

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

ENV BOOST_HOME="/usr/lib/boost"

ARG BOOST_VERSION="1.75.0"
ARG BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost"

RUN mkdir -p "${BOOST_HOME}" && \
    boost_archive_url="${BOOST_URL}/${BOOST_VERSION}/boost-${BOOST_VERSION}-x64-gcc9-musl-static-runtime.tar.gz" && \
    echo "Downloading Boost from ${boost_archive_url} into ${BOOST_HOME}" && \
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
      -D CMAKE_USER_MAKE_RULES_OVERRIDE="${source_dir}/cmake/static_c_runtime_overrides.cmake" \
      -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="${source_dir}/cmake/static_cxx_runtime_overrides.cmake" \
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

FROM scratch

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]

COPY --from=build ["/opt/ma_echo_server", "/opt/ma_echo_server"]
