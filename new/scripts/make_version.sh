#!/usr/bin/env bash

set -e

VERSION=$(cat $(git rev-parse --show-toplevel)/VERSION)
TAG="v${VERSION}.0"
COMMITS_SINCE=$(git log --oneline ${TAG}..HEAD | wc -l)

GIT_HEAD=$(git rev-parse HEAD)
GIT_DIRTY=''
if [[ $(git ls-files -m | wc -l) -ne 0 ]]; then
    GIT_DIRTY='-dirty'
fi

FINGERPRINT=$(cat $(find ./include ./src -type f | sort) | openssl dgst -sha3-256 | cut -d' ' -f1)

MODE=${1}
case ${MODE} in
    full)
        echo "${VERSION}.${COMMITS_SINCE} (${GIT_HEAD}${GIT_DIRTY})"
        ;;
    fuller)
        echo "${VERSION}.${COMMITS_SINCE}-${GIT_HEAD}${GIT_DIRTY} (${FINGERPRINT})"
        ;;
    git)
        echo "${GIT_HEAD}${GIT_DIRTY}"
        ;;
    fingerprint|fp)
        echo "${FINGERPRINT}"
        ;;
    base)
        echo "${VERSION}.0"
        ;;
    *)
        echo "${VERSION}.${COMMITS_SINCE}"
        ;;
esac
