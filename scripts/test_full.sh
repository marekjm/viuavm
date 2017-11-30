#!/usr/bin/env bash

set -e
set -x

# First, test with sanitisers.
echo "Running sanitised build"
make clean
make "$@"
VIUA_TEST_SUITE_VALGRIND_CHECKS=0 make test

# Then, test without sanitisers but with Valgrind.
echo "Running Valgrind build"
make clean
make GENERIC_SANITISER_FLAGS= CLANG_SANITISER_FLAGS= GCC_SANITISER_FLAGS= "$@"
make test

if [[ $OPTIMISED == '' ]]; then
    OPTIMISED=yes
fi
if [[ $OPTIMISED == 'yes' ]]; then
    # Then, test with optimisations enabled, and
    # without either Valgrind or sanitisers slowing the machine down.
    # Note that compiling Viua with optimisations enabled requires a machine with more
    # about 8GB of RAM.
    echo "Running optimised build"
    make clean
    make GENERIC_SANITISER_FLAGS= CLANG_SANITISER_FLAGS= GCC_SANITISER_FLAGS= CXXOPTIMIZATIONFLAGS=-O3 "$@"
    VIUA_TEST_SUITE_VALGRIND_CHECKS=0 make test
fi
