#!/usr/bin/env sh

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

g++ -std=c++11 -fPIC -c -o exception.o src/types/exception.cpp
g++ -std=c++11 -fPIC -c -o vector.o src/types/vector.cpp
g++ -std=c++11 -fPIC -c -o registerset.o src/cpu/registerset.cpp
g++ -std=c++11 -fPIC -c -o support_string.o src/support/string.cpp
g++ -std=c++11 -fPIC -c -o $ONAME $1
g++ -std=c++11 -fPIC -shared -o $SONAME $ONAME exception.o vector.o registerset.o support_string.o

rm exception.o vector.o registerset.o support_string.o $ONAME
