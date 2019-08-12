#!/bin/bash

set -e

echo -n \
  | openssl s_client -connect scan.coverity.com:443 \
  | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' \
  | sudo tee -a /etc/ssl/certs/ca-certificates.crt
