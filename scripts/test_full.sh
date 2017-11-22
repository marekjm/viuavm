#!/usr/bin/env bash

set -e
set -x

# First, test with sanitisers.
make clean
make "$@"
VIUA_TEST_SUITE_VALGRIND_CHECKS=0 make test

# Then, test without sanitisers but with Valgrind.
make clean
make GENERIC_SANITISER_FLAGS= CLANG_SANITISER_FLAGS= GCC_SANITISER_FLAGS= "$@"
make test

# Then, test with optimisations enabled, and
# without either Valgrind or sanitisers slowing the machine down.
make clean
make GENERIC_SANITISER_FLAGS= CLANG_SANITISER_FLAGS= GCC_SANITISER_FLAGS= CXXOPTIMIZATIONFLAGS=-O3 "$@"
VIUA_TEST_SUITE_VALGRIND_CHECKS=0 make test
