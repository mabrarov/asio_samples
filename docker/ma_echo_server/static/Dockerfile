#
# Copyright (c) 2020 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine:3.15.0 AS build

RUN apk add --no-cache \
      bash \
      ca-certificates \
      curl \
      tar \
      gzip \
      git \
      g++ \
      make \
      cmake \
      libstdc++ \
      linux-headers \
      icu-dev \
      python2-dev \
      libzip-dev \
      libbz2 \
      dos2unix \
      patch

ADD ["patch", "/opt/patch"]

ARG BOOST_VERSION="1.78.0"
ARG BOOST_RELEASE_URL="https://boostorg.jfrog.io/artifactory/main/release"
ARG BOOST_BUILD_OPTIONS="--without-mpi --without-graph_parallel"

ENV BOOST_INSTALL_DIR="/opt/boost" \
    BOOST_PATCH_DIR="/opt/patch"

RUN mkdir -p "${BOOST_INSTALL_DIR}" && \
    boost_build_dir="$(mktemp -d)" && \
    boost_download_url="${BOOST_RELEASE_URL}/${BOOST_VERSION}/source/boost_$(echo "${BOOST_VERSION}" | sed -r 's/\./_/g').tar.gz" && \
    echo "Downloading Boost C++ Libraries (source code archive) from ${boost_download_url} into ${boost_build_dir} directory" && \
    curl -jksSL "${boost_download_url}" | tar -xzf - -C "${boost_build_dir}" --strip-components 1 && \
    b2_bin="${boost_build_dir}/b2" && \
    b2_toolset="gcc" && \
    boost_bootstrap="${boost_build_dir}/bootstrap.sh" && \
    current_dir="$(pwd)" && \
    cd "${boost_build_dir}" && \
    boost_patch_file="${BOOST_PATCH_DIR}/boost-${BOOST_VERSION}.patch" && \
    if [[ -f "${boost_patch_file}" ]]; then \
      echo "Patching Boost C++ Libraries using ${boost_patch_file}" && \
      dos2unix <"${boost_patch_file}" | patch -uNf -p0 ; \
    fi && \
    echo "Building Boost.Build engine" && \
    "${boost_bootstrap}" && \
    boost_linkage="static" && \
    boost_runtime_linkage="static" && \
    echo "Building Boost C++ Libraries with these parameters:" && \
    echo "B2_BIN               : ${b2_bin}" && \
    echo "B2_TOOLSET           : ${b2_toolset}" && \
    echo "BOOST_INSTALL_DIR    : ${BOOST_INSTALL_DIR}" && \
    echo "BOOST_LINKAGE        : ${boost_linkage}" && \
    echo "BOOST_RUNTIME_LINKAGE: ${boost_runtime_linkage}" && \
    echo "BOOST_BUILD_OPTIONS  : ${BOOST_BUILD_OPTIONS}" && \
    "${b2_bin}" \
      --toolset="${b2_toolset}" \
      link="${boost_linkage}" \
      runtime-link="${boost_runtime_linkage}" \
      install \
      --prefix="${BOOST_INSTALL_DIR}" \
      --layout=system \
      ${BOOST_BUILD_OPTIONS} && \
    cd "${current_dir}" && \
    rm -rf "${boost_build_dir}"

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
      -D BOOST_INCLUDEDIR="${BOOST_INSTALL_DIR}/include" \
      -D BOOST_LIBRARYDIR="${BOOST_INSTALL_DIR}/lib" \
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

FROM gcr.io/distroless/static-debian11

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]

COPY --from=build ["/opt/ma_echo_server", "/opt/ma_echo_server"]
