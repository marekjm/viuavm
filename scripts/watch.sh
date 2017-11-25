#!/usr/bin/env bash

set -e

CXX_EXTRA_FLAGS=-fdiagnostics-color=always
export CXX_EXTRA_FLAGS

while true; do
    (find ./include -name '*.h' ; find ./src/ -name '*.cpp') | entr -p ./scripts/compile_and_notify.sh
done
