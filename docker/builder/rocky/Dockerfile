#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM rockylinux:9

LABEL name="abrarov/asio-samples-builder-rocky" \
    description="Builder of Asio samples project on Rocky Linux"

ENV PROJECT_DIR="/project" \
    BUILD_TYPE="RELEASE" \
    STATIC_RUNTIME="OFF" \
    BOOST_USE_STATIC_LIBS="ON" \
    MA_QT="ON" \
    MA_QT_MAJOR_VERSION="5" \
    MA_COVERAGE="OFF" \
    CMAKE_VERSION="3.30.5" \
    PATH="/opt/cmake/bin:${PATH}"

ENTRYPOINT ["/app/start.sh"]

RUN dnf check-update -y || true && \
    dnf install --setopt=tsflags=nodocs -y epel-release && \
    crb enable && \
    dnf check-update -y || true && \
    dnf groupinstall --setopt=tsflags=nodocs -y 'Development Tools' && \
    dnf install --setopt=tsflags=nodocs -y ca-certificates && \
    dnf install --setopt=tsflags=nodocs -y --allowerasing curl && \
    mkdir -p /opt/cmake && \
    curl -jksSL \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" \
      | tar -xzf - -C /opt/cmake --strip-components 1 && \
    dnf install --setopt=tsflags=nodocs -y \
      glibc-static \
      libstdc++-static \
      lcov \
      boost-devel \
      boost-static \
      qt5-qtbase-devel && \
    dnf clean -y all && \
    mkdir /project && \
    mkdir /build

WORKDIR /build

ADD ["start.sh", "/app/"]
