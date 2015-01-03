#!/usr/bin/env sh

set -e

if [[ $1 == "" ]]; then
    echo "fatal: requires path"
    exit 1
fi


for src in $1/*.asm; do
    echo -n "$src : "
    ./scripts/run-sample.sh $src
done
