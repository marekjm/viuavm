#!/usr/bin/env bash

VERSION=$(cat $(git rev-parse --show-toplevel)/VERSION)

GIT_HEAD=$(git rev-parse HEAD)
GIT_DIRTY=''
if [[ $(git ls-files -m | wc -l) -ne 0 ]]; then
    GIT_DIRTY='-dirty'
fi

FINGERPRINT=$(cat $(find ./include ./src -type f) | sha384sum | cut -d' ' -f1)

MODE=${1}
case ${MODE} in
    full)
        echo "${VERSION} (${GIT_HEAD}${GIT_DIRTY})"
        ;;
    fuller)
        echo "${VERSION}-${GIT_HEAD}${GIT_DIRTY} (${FINGERPRINT})"
        ;;
    git)
        echo "${GIT_HEAD}${GIT_DIRTY}"
        ;;
    fingerprint|fp)
        echo "${FINGERPRINT}"
        ;;
    *)
        echo "${VERSION}"
        ;;
esac
