#
# Copyright (c) 2017 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

FROM microsoft/nanoserver:10.0.14393.1198

LABEL name="abrarov/tcp-echo" \
    description="TCP echo server from Asio samples project" \
    license="BSL-1.0"

ENV BUILD_ID=bjnu35iauc1bl0od

RUN powershell "New-Item \"$env:ProgramFiles\ma_echo_server\" -type directory | out-null" && \
    powershell "Invoke-WebRequest -Uri \"https://ci.appveyor.com/api/buildjobs/$env:BUILD_ID/artifacts/build%2Fexamples%2Fma_echo_server%2FRelease%2Fma_echo_server.exe\" -OutFile \"$env:ProgramFiles\ma_echo_server\ma_echo_server.exe\""

ENTRYPOINT ["C:/Program Files/ma_echo_server/ma_echo_server.exe"]
