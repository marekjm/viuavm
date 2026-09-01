#!/usr/bin/env sh

set -e

viua asm -o ln.o ln.asm
viua ld -o ln.elf ln.o tests/std/abs.o tests/std/pow.o
viua vm ln.elf
