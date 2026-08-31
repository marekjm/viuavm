#!/usr/bin/env sh

set -e

viua asm -o ln.o ln.asm
viua ld -o ln.elf ln.o
viua vm ln.elf
