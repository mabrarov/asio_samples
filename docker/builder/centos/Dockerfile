#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM centos:7

LABEL name="abrarov/asio-samples-builder-centos" \
    description="Builder of Asio samples project on CentOS"

ENV PROJECT_DIR="/project" \
    BUILD_TYPE="RELEASE" \
    STATIC_RUNTIME="OFF" \
    BOOST_USE_STATIC_LIBS="ON" \
    MA_QT="ON" \
    MA_QT_MAJOR_VERSION="5" \
    MA_COVERAGE="OFF" \
    CMAKE_VERSION="3.18.2" \
    PATH="/opt/cmake/bin:${PATH}"

ADD ["start.sh", "/app/"]

RUN yum update -y && \
    yum install --setopt=tsflags=nodocs -y epel-release && \
    yum groupinstall --setopt=tsflags=nodocs -y 'Development Tools' && \
    yum install --setopt=tsflags=nodocs -y curl && \
    mkdir -p /opt && \
    curl -jksSL \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" \
      | tar -xzf - -C /opt && \
    mv -f "/opt/cmake-${CMAKE_VERSION}-Linux-x86_64" /opt/cmake && \
    yum install --setopt=tsflags=nodocs -y glibc-static && \
    yum install --setopt=tsflags=nodocs -y libstdc++-static && \
    yum install --setopt=tsflags=nodocs -y lcov && \
    yum install --setopt=tsflags=nodocs -y boost-devel && \
    yum install --setopt=tsflags=nodocs -y boost-static && \
    yum install --setopt=tsflags=nodocs -y qt-devel && \
    yum install --setopt=tsflags=nodocs -y qt5-qtbase-devel && \
    yum clean -y all && \
    mkdir /project && \
    mkdir /build

VOLUME ["/project", "/build"]

WORKDIR /build

ENTRYPOINT ["/app/start.sh"]
