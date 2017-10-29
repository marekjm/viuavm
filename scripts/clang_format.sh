#!/usr/bin/env bash

find ./include -name '*.h' ; find ./src -name '*.cpp' | xargs -n 1 --verbose clang-format -i
