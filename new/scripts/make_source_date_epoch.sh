#!/bin/sh

set -e

if [ ! -z ${SOURCE_DATE_EPOCH} ]; then
    echo "${SOURCE_DATE_EPOCH}"
    exit 0
fi

git show --format='%ct' HEAD | head -n 1
