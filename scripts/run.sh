#!/usr/bin/zsh

##############################################################
#
#   This file is a compile-and-run frontend for Viua VM.
#   It first calls an assembler on given file and then
#   runs the output inside the CPU.
#
#   Environment variables listed below affect this script.
#   They are intended to be used in such a form:
#
#       ENV_VAR=value ./scripts/run.sh <file>
#
#   and are a primitive form of user interface for this
#   frontend.
#
#       DEBUG_ASM       - if passed as 1, the script will pass
#                         the --debug flag to assembler
#
#       DEBUG_CPU       - if passed as 1, the script will pass
#                         the --debug flag to CPU
#
#       DEBUG_VIUA     - if passed as 1, both assembler and
#                         the CPU will receive --debug flag
#
#
##############################################################

set -e

VIUA_ASM=./build/bin/vm/asm
VIUA_CPU=./build/bin/vm/cpu

if [[ $1 == "" ]]; then
    # must pass a filename to run
    echo "fatal: no file to run"
    exit 1
fi

if [[ $DEBUG_VIUA == 1 ]]; then
    # DEBUG_VIUA just sets the other to up
    # in a correct way
    DEBUG_ASM=1
    DEBUG_CPU=1
fi

if [[ $DEBUG_ASM == 1 ]]; then
    $VIUA_ASM --debug -o `basename $1.bin` $1
else
    $VIUA_ASM -o `basename $1.bin` $1 
fi

if [[ $DEBUG_CPU == 1 ]]; then
    $VIUA_CPU --debug `basename $1.bin`
else
    $VIUA_CPU `basename $1.bin`
fi
