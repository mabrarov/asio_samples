#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM abrarov/msvc-2015:2.5.0 as build

ARG MA_REVISION=master

ENV MSVC_VERSION="14.0" \
    BOOST_VERSION="1.70.0" \
    BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost" \
    SOURCE_DIR="C:\asio_samples" \
    BUILD_DIR="C:\build" \
    DEPENDENCIES_DIR="C:\dependencies" \
    DOWNLOADS_DIR="C:\downloads"

ADD ["app", "C:/app/"]

RUN powershell -ExecutionPolicy Bypass -File "C:\app\build.ps1"

FROM microsoft/nanoserver:10.0.14393.1198

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

COPY --from=build "C:\\build\\examples\\ma_echo_server\\Release\\ma_echo_server.exe" "C:\\ma_echo_server.exe"

ENTRYPOINT ["C:\\ma_echo_server.exe"]
