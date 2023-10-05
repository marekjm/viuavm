#!/usr/bin/env bash

FILE=${1}
if [[ ! -f "${FILE}" ]]; then
    1>&2 echo "error: not a file: ${FILE}"
    exit 2
fi

FEATURE_NAME=$(basename ${FILE} | sed 's/\(.*\)\.c\(pp\|xx\)*/\U\1/')

if [[ -z ${CXX} ]]; then
    1>&2 echo "error: CXX not set"
    exit 3
fi

${CXX} -o /dev/null ${FILE} 2>/dev/null
if [[ $? -eq 0 ]]; then
    echo "-DVIUA_PLATFORM_HAS_FEATURE_${FEATURE_NAME}"
fi
