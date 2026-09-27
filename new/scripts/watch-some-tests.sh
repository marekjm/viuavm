#!/usr/bin/env bash

set -e
set -x

(ls -1 tests/suite.py ; echo "${@}") |
    tr ' ' '\n' |
    entr -c bash ./scripts/run-some-tests.sh "${@}"
