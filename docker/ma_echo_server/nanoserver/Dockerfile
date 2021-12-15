#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM abrarov/msvc-2019:2.13.0 AS build

ENV DOWNLOADS_DIR="C:\download" \
    BOOST_DIR="C:\dependencies\boost"

ADD ["install", "C:/install/"]

ARG BOOST_VERSION="1.78.0"
ARG BOOST_URL="https://boostorg.jfrog.io/ui/api/v1/download?repoKey=main&path="

RUN powershell -ExecutionPolicy Bypass -File "C:\install\install.ps1"

ENV SOURCE_DIR="C:\asio_samples" \
    BUILD_DIR="C:\build"

ADD ["app", "C:/app/"]

ARG MA_REVISION=master

RUN powershell -ExecutionPolicy Bypass -File "C:\app\build.ps1"

FROM mcr.microsoft.com/windows/nanoserver:1809

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

ENTRYPOINT ["C:\\ma_echo_server.exe"]

COPY --from=build \
    ["C:\\\\build\\\\examples\\\\ma_echo_server\\\\Release\\\\ma_echo_server.exe", \
     "C:\\\\ma_echo_server.exe"]
