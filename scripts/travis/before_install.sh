#!/bin/bash

#
# Copyright (c) 2019 Marat Abrarov (abrarov@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#

set -e

echo -n \
  | openssl s_client -connect scan.coverity.com:443 \
  | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' \
  | sudo tee -a /etc/ssl/certs/ca-certificates.crt > /dev/null
