#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM abrarov/msvc-2019:2.9.0 AS build

ARG MA_REVISION=master

ENV CMAKE_VERSION="3.18.2" \
    BOOST_VERSION="1.74.0" \
    BOOST_URL="https://dl.bintray.com/mabrarov/generic/boost" \
    SOURCE_DIR="C:\asio_samples" \
    BUILD_DIR="C:\build" \
    DEPENDENCIES_DIR="C:\dependencies" \
    DOWNLOADS_DIR="C:\downloads"

ADD ["app", "C:/app/"]

RUN powershell -ExecutionPolicy Bypass -File "C:\app\build.ps1"

FROM mcr.microsoft.com/windows/nanoserver:1809

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

COPY --from=build "C:\\build\\examples\\ma_echo_server\\Release\\ma_echo_server.exe" "C:\\ma_echo_server.exe"

ENTRYPOINT ["C:\\ma_echo_server.exe"]
