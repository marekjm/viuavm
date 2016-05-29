#!/usr/bin/env sh

# This script does not set -e flag because we *want* to tolerate errors
# raised by `taskset ...` command.

if [[ $VVM_DEBUG_LOG == '' ]]; then
    VVM_DEBUG_LOG=/tmp/log.txt
fi
if [[ $VVM_DEBUG_LOG_SHORT == '' ]]; then
    VVM_DEBUG_LOG_SHORT=/tmp/log.txt.log
fi

if [[ $VVM_DEBUG_CPUS == '' ]]; then
    VVM_DEBUG_CPUS=0
fi
if [[ $VVM_DEBUG_TIMEOUT == '' ]]; then
    VVM_DEBUG_TIMEOUT=5
fi

if [[ $VVM_DEBUG_FILE == '' ]]; then
    VVM_DEBUG_FILE=$1
fi
if [[ $VVM_DEBUG_FILE == '' ]]; then
    VVM_DEBUG_FILE=a.out
fi

if [[ ! -f $VVM_DEBUG_FILE ]]; then
    echo "error: not a file: $VVM_DEBUG_FILE"
    exit 1
fi

# First, log output.
if [[ $VVM_DEBUG_TIMEOUT ]]; then
    timeout $VVM_DEBUG_TIMEOUT taskset -a -c $VVM_DEBUG_CPUS ./build/bin/vm/cpu $VVM_DEBUG_FILE > $VVM_DEBUG_LOG
else
    taskset -a -c $VVM_DEBUG_CPUS ./build/bin/vm/cpu $VVM_DEBUG_FILE > $VVM_DEBUG_LOG
fi

# Then, shorten it.
./build/bin/tools/log-shortener $VVM_DEBUG_LOG > $VVM_DEBUG_LOG_SHORT
