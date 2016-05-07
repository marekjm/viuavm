#!/usr/bin/env sh

CXXFLAGS='-Wall -Wextra -Wzero-as-null-pointer-constant -Wuseless-cast -Wconversion -Winline -pedantic -Wfatal-errors -g -I./include'

if [[ $(g++ --version | head -n 1 | grep -Po 'g\+\+ \(GCC\) 6' | cat) == 'g++ (GCC) 6' ]]; then
    CXXFLAGS="-std=c++14 $CXXFLAGS"
else
    CXXFLAGS="-std=c++11 $CXXFLAGS"
fi

LINES=$(wc -l Makefile | cut -d' ' -f1)
LINES=$(dc -e "$LINES 1 - p")

echo "CXXFLAGS=$CXXFLAGS"
cat Makefile | tail -n $LINES
