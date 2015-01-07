#!/usr/bin/zsh

##############################################################
#
#   This script will run all files in a given directory
#   so it's easy to check if any of them gives invalid output.
#
#   It acts as a primitive test runner and
#   will be replaced in the future by a proper test suite.
#
##############################################################

set -e

if [[ $1 == "" ]]; then
    # require a directory in which .asm scripts are placed
    echo "fatal: requires path"
    exit 1
fi


for src in $1/*.asm; do
    echo -n "$src : "
    ./scripts/run-sample.sh $src
done
