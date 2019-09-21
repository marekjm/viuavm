#!/usr/bin/env bash

set -e

export VIUA_LIBRARY_PATH=./build/stdlib
export VIUA_PROC_SCHEDULERS=4
export VIUA_IO_SCHEDULERS=1
export VIUA_FFI_SCHEDULERS=4

# ./build/bin/vm/asm --no-sa net_server.asm
./build/bin/vm/asm net_server.asm
./build/bin/vm/kernel a.out "$@"
