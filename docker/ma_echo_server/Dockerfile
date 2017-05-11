#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM alpine

RUN apk add --no-cache g++ make cmake boost-dev git libstdc++ &&\
    cd /tmp &&\
    git clone https://github.com/mabrarov/asio_samples.git &&\
    cd asio_samples &&\
    mkdir build &&\
    cd build &&\
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D Boost_USE_STATIC_LIBS=ON -D ma_qt=OFF .. &&\
    make ma_echo_server &&\
    mkdir -p /opt/ma_echo_server &&\
    mv -f examples/ma_echo_server/ma_echo_server /opt/ma_echo_server/ma_echo_server &&\
    cd / &&\
    rm -rf /tmp &&\
    apk del git boost-dev cmake make g++

ENTRYPOINT ["/opt/ma_echo_server/ma_echo_server"]
