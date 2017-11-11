#!/usr/bin/env bash

set -e

while true; do
    (find ./include -name '*.h' ; find ./src/ -name '*.cpp') | entr -p ./scripts/compile_and_notify.sh
done
