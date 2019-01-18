#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM ubuntu:18.04

LABEL name="abrarov/asio-samples-builder-ubuntu" \
    description="Builder of Asio samples project on Ubuntu"

ENV PROJECT_DIR="/project" \
    BUILD_TYPE=RELEASE \
    BOOST_USE_STATIC_LIBS=ON \
    MA_QT=ON \
    MA_QT_MAJOR_VERSION=5 \
    COVERAGE_BUILD=OFF

ADD ["start.sh", "/app/"]

RUN apt-get update -y &&\
    apt-get install -y build-essential cmake lcov libboost-all-dev libqt4-dev qtbase5-dev &&\
    rm -rf /var/lib/apt/lists/* &&\
    tr -d '\015' < /app/start.sh > /app/start-lf.sh &&\
    mv -f /app/start-lf.sh /app/start.sh &&\
    chmod u+x /app/start.sh &&\
    mkdir /project &&\
    mkdir /build

VOLUME ["/project", "/build"]

WORKDIR /build

ENTRYPOINT ["/app/start.sh"]
