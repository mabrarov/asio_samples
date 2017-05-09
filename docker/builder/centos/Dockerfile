#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM centos:7

LABEL name="abrarov/asio-samples-builder-centos" \
    description="Builder of Asio samples project on CentOS"

ENV PROJECT_DIR="/project" \
    BUILD_TYPE=RELEASE \
    BOOST_USE_STATIC_LIBS=ON \
    MA_QT=ON \
    MA_QT_MAJOR_VERSION=5 \
    COVERAGE_BUILD=OFF \
    PATH=/opt/cmake/bin:${PATH}

ADD ["start.sh", "/app/"]

RUN yum -y update &&\
    yum -y install epel-release &&\
    yum -y groupinstall 'Development Tools' &&\
    yum -y install curl &&\
        curl -jksSL https://cmake.org/files/v3.8/cmake-3.8.1-Linux-x86_64.tar.gz | tar -xzf - -C /opt &&\
        mv -f /opt/cmake-3.8.1-Linux-x86_64 /opt/cmake &&\
    yum -y install lcov boost-devel boost-static qt-devel qt5-qtbase-devel &&\
    yum -y clean all &&\
    tr -d '\015' < /app/start.sh > /app/start-lf.sh &&\
    mv -f /app/start-lf.sh /app/start.sh &&\
    chmod u+x /app/start.sh &&\
    mkdir /project &&\
    mkdir /build

VOLUME ["/project", "/build"]

WORKDIR /build

ENTRYPOINT ["/app/start.sh"]
