#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM abrarov/asio-samples-builder-alpine-base

LABEL name="abrarov/asio-samples-builder-alpine-qt5" \
    description="Builder of Asio samples project on Alpine Linux with Qt 5.x"

ENV MA_QT=ON \
    MA_QT_MAJOR_VERSION=5

RUN apk add --no-cache qt5-qtbase-dev
