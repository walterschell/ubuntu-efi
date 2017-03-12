#!/bin/bash
openssl genrsa -out test-key.rsa 2048
openssl req -new -x509 -sha256 \
        -subj '/CN=test-key' -key test-key.rsa -out test-cert.pem
openssl x509 -in test-cert.pem -inform PEM \
        -out test-cert.der -outform DER

