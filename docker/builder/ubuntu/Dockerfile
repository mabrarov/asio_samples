#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM ubuntu:20.04

LABEL name="abrarov/asio-samples-builder-ubuntu" \
    description="Builder of Asio samples project on Ubuntu"

ENV TZ="Europe/Moscow" \
    PROJECT_DIR="/project" \
    BUILD_TYPE="RELEASE" \
    BOOST_USE_STATIC_LIBS="ON" \
    MA_QT="ON" \
    MA_QT_MAJOR_VERSION="5" \
    MA_COVERAGE="OFF" \
    CMAKE_VERSION="3.18.2" \
    PATH="/opt/cmake/bin:${PATH}"

ADD ["start.sh", "/app/"]

RUN ln -snf "/usr/share/zoneinfo/${TZ}" /etc/localtime && \
      echo "${TZ}" > /etc/timezone && \
    apt-get update && \
    apt-get install -y \
      curl \
      build-essential \
      lcov \
      libboost-all-dev \
      qtbase5-dev && \
    mkdir -p /opt && \
    curl -jksSL \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz" \
      | tar -xzf - -C /opt && \
    mv -f "/opt/cmake-${CMAKE_VERSION}-Linux-x86_64" /opt/cmake && \
    rm -rf /var/lib/apt/lists/* && \
    mkdir /project && \
    mkdir /build

VOLUME ["/project", "/build"]

WORKDIR /build

ENTRYPOINT ["/app/start.sh"]
