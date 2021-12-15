#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine:3.15.0

LABEL name="abrarov/asio-samples-builder-alpine" \
    description="Builder of Asio samples project on Alpine Linux"

ENV PROJECT_DIR="/project" \
    BUILD_TYPE="RELEASE" \
    BOOST_USE_STATIC_LIBS="ON" \
    MA_QT="ON" \
    MA_QT_MAJOR_VERSION="5" \
    MA_COVERAGE="OFF"

ENTRYPOINT ["/app/start.sh"]

RUN echo 'http://nl.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories && \
    apk add --no-cache \
      g++ \
      make \
      cmake \
      lcov \
      musl-dev \
      libgcc \
      libstdc++ \
      boost-dev \
      boost-static \
      qt5-qtbase-dev && \
    mkdir /project && \
    mkdir /build

WORKDIR /build

ADD ["start.sh", "/app/"]
