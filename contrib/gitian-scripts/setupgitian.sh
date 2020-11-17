#!/usr/bin/env bash
export LC_ALL=C
export VERSION=master
export SIGNER='Falcon'
./gitian-build.py --setup $SIGNER $VERSION
