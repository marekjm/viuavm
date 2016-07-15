#!/usr/bin/env sh

#
#   Copyright (C) 2016 Marek Marecki
#
#   This file is part of Viua VM.
#
#   Viua VM is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Viua VM is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
#

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
