#!/usr/bin/env bash

set -e

python3 ./tests/suite.py $(echo "$@" |
    tr ' ' '\n' |
    sed -e 's#.*/##' -e 's/\..*$//' |
    sort |
    uniq)
