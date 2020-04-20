#!/usr/bin/env bash

set -e

PT_PATH=misc/platformy_technologiczne

export VIUA_PROC_SCHEDULERS=2
export VIUA_FFI_SCHEDULERS=2
export VIUA_IO_SCHEDULERS=1
export VIUA_LIBRARY_PATH=./build/stdlib:$PT_PATH

touch $PT_PATH/libviuapq.cpp.hash
if [[ ! -f $PT_PATH/viuapq.so ]]; then
    echo '0' > $PT_PATH/libviuapq.cpp.hash
fi
if [[ $(cat $PT_PATH/libviuapq.cpp.hash) != $(sha384sum $PT_PATH/libviuapq.cpp) ]]; then
    gxx \
        -fPIC \
        -shared \
        -I/home/maelkum/.local/include \
        -o $PT_PATH/viuapq.so \
        $PT_PATH/libviuapq.cpp \
        -lpq
    sha384sum $PT_PATH/libviuapq.cpp > $PT_PATH/libviuapq.cpp.hash
fi

cat $PT_PATH/async.sql | psql pt_lab3

touch $PT_PATH/async.asm.hash
if [[ ! -f ./async.bin ]]; then
    echo '0' > $PT_PATH/async.asm.hash
fi
if [[ $(cat $PT_PATH/async.asm.hash) != $(sha384sum $PT_PATH/async.asm) ]]; then
    ./build/bin/vm/asm -o ./async.bin $PT_PATH/async.asm
    sha384sum $PT_PATH/async.asm > $PT_PATH/async.asm.hash
fi

# gdb --args ./build/bin/vm/kernel ./async.bin
# valgrind --leak-check=full ./build/bin/vm/kernel ./async.bin
./build/bin/vm/kernel ./async.bin || true
./build/bin/vm/kernel --info --verbose
