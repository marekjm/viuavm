#!/usr/bin/env bash

MODE=$1

if [[ $MODE == 'git' ]]; then
    git ls-files -m | grep -P '\.(h|cpp)$' | xargs -n 1 --verbose clang-format -i
else
    (find ./include -name '*.h' ; find ./src -name '*.cpp') | xargs -n 1 --verbose clang-format -i
fi
