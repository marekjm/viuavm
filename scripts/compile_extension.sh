#!/usr/bin/env sh

#
#   Copyright (C) 2015, 2016 Marek Marecki
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

set -e

if [[ $1 == "--help" ]]; then
    echo "SYNTAX

    $0 --help
    $0 <path> [<output>]

DESCRIPTION

    This script takes a path to single C++ extension source file and
    creates a shared object loadable by Viua.

    Output path can be ommitted.
    By default it becomes basename of the input, with extension changed to '.so'.
    "
    exit 0
fi

if [[ $1 == "" ]]; then
    echo "no input file"
    exit 1
fi
if [[ ! -f $1 ]]; then
    echo "not a file: $1"
    exit 1
fi

BASENAME=$(basename $1 | sed 's/\.[a-zA-Z0-9]*$//')
ONAME="$BASENAME.o"
SONAME="$BASENAME.so"

if [[ $2 != "" ]]; then
    $SONAME=$2
fi

g++ -std=c++14 -fPIC -c -o exception.o src/types/exception.cpp
g++ -std=c++14 -fPIC -c -o vector.o src/types/vector.cpp
g++ -std=c++14 -fPIC -c -o registerset.o src/cpu/registerset.cpp
g++ -std=c++14 -fPIC -c -o support_string.o src/support/string.cpp
g++ -std=c++14 -fPIC -c -o $ONAME $1
g++ -std=c++14 -fPIC -shared -o $SONAME $ONAME exception.o vector.o registerset.o support_string.o

rm exception.o vector.o registerset.o support_string.o $ONAME
