#!/usr/bin/zsh

##############################################################
#
#   This file is a compile-and-run frontend for Wudoo VM.
#   It first calls an assembler on given file and then
#   runs the output inside the CPU.
#
#   The bin/vm/run binary name is a bit misleading as it
#   is not a "runner" for code but a frontend which is
#   just loading bytecode into the CPU and kicks it
#   so it starts running the bytecode.
#
#   Environment variables listed below affect this script.
#   They are intended to be used in such a form:
#
#       ENV_VAR=value ./scripts/run-sample.sh <file>
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
#       DEBUG_WUDOO     - if passed as 1, both assembler and
#                         the CPU will receive --debug flag
#
#
##############################################################

set -e

if [[ $1 == "" ]]; then
    # must pass a filename to run
    echo "fatal: no file to run"
    exit 1
fi

if [[ $DEBUG_WUDOO == 1 ]]; then
    # DEBUG_WUDOO just sets the other to up
    # in a correct way
    DEBUG_ASM=1
    DEBUG_CPU=1
fi

if [[ $DEBUG_ASM == 1 ]]; then
    bin/vm/asm --debug $1 bin/sample/`basename $1.bin`
else
    bin/vm/asm $1 bin/sample/`basename $1.bin`
fi

if [[ $DEBUG_CPU == 1 ]]; then
    bin/vm/cpu --debug bin/sample/`basename $1.bin`
else
    bin/vm/cpu bin/sample/`basename $1.bin`
fi
