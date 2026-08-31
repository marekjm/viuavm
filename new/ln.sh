#!/usr/bin/env sh

set -e

viua asm -o ln.o ln.asm
viua ld -o ln.elf ln.o tests/std/abs.o
viua vm ln.elf
