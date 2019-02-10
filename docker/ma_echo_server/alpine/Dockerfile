#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine:3.9

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

RUN apk add --no-cache libstdc++ &&\
    apk add --no-cache --virtual build-dependencies g++ make cmake boost-dev boost-static git libstdc++ &&\
    cd /tmp &&\
    git clone https://github.com/mabrarov/asio_samples.git &&\
    cd asio_samples &&\
    mkdir build &&\
    cd build &&\
    cmake -D CMAKE_SKIP_BUILD_RPATH=ON \
          -D CMAKE_BUILD_TYPE=RELEASE \
          -D Boost_USE_STATIC_LIBS=ON \
          -D ma_qt=OFF \
          .. &&\
    cmake --build . --target ma_echo_server &&\
    mkdir -p /opt/ma_echo_server &&\
    mv -f examples/ma_echo_server/ma_echo_server /opt/ma_echo_server/ma_echo_server &&\
    cd /tmp &&\
    rm -rf asio_samples &&\
    apk del build-dependencies

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]
