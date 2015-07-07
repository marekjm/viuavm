#!/usr/bin/env sh

FILE=src/version.h
sed -i "s/COMMIT = \".*\"/COMMIT = \"`git rev-parse HEAD`\"/" $FILE
sed -i "s/MICRO = \".*\"/MICRO = \"`git describe HEAD | cut -d'-' -f2`\"/" $FILE
