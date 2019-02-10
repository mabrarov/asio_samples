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
    BUILD_TYPE=RELEASE \
    STATIC_RUNTIME=OFF \
    BOOST_USE_STATIC_LIBS=ON \
    MA_QT=ON \
    MA_QT_MAJOR_VERSION=5 \
    COVERAGE_BUILD=OFF \
    CMAKE_VERSION=3.13.3 \
    PATH=/opt/cmake/bin:${PATH}

ADD ["start.sh", "/app/"]

RUN yum update -y &&\
    yum install -y epel-release &&\
    yum groupinstall -y 'Development Tools' &&\
    yum install -y curl &&\
    mkdir -p /opt &&\
    curl -jksSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz | tar -xzf - -C /opt &&\
    mv -f /opt/cmake-${CMAKE_VERSION}-Linux-x86_64 /opt/cmake &&\
    yum install -y glibc-static &&\
    yum install -y libstdc++-static &&\
    yum install -y lcov &&\
    yum install -y boost-devel &&\
    yum install -y boost-static &&\
    yum install -y qt-devel &&\
    yum install -y qt5-qtbase-devel &&\
    yum clean -y all &&\
    tr -d '\015' < /app/start.sh > /app/start-lf.sh &&\
    mv -f /app/start-lf.sh /app/start.sh &&\
    chmod u+x /app/start.sh &&\
    mkdir /project &&\
    mkdir /build

VOLUME ["/project", "/build"]

WORKDIR /build

ENTRYPOINT ["/app/start.sh"]
