#!/usr/bin/env bash

MODE=$1

CPUS_AVAILABLE=$(sed 's/^processor[\t ]*: *\([0-9]*\)/\1/;t;d' /proc/cpuinfo | tail -n 1)

if [[ $MODE == 'git' ]]; then
    git ls-files -m |
        grep -P '\.(h|cpp)$' |
        xargs -n 1 -P $CPUS_AVAILABLE --verbose clang-format -i
else
    (find ./include -name '*.h' ; find ./src -name '*.cpp') |
        xargs -n 1 -P $CPUS_AVAILABLE --verbose clang-format -i
fi
