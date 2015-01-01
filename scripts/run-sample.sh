#!/usr/bin/zsh

set -e

if [[ $1 == "" ]]; then
    echo "fatal: no file to run"
    exit 1
fi

if [[ $DEBUG_ASM == 1 ]]; then
    bin/vm/asm --debug $1 bin/sample/`basename $1.bin`
else
    bin/vm/asm $1 bin/sample/`basename $1.bin`
fi

if [[ $DEBUG_CPU == 1 ]]; then
    bin/vm/run --debug bin/sample/`basename $1.bin`
else
    bin/vm/run bin/sample/`basename $1.bin`
fi
