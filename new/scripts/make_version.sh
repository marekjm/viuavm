#!/bin/sh

set -e

VERSION=$(cat $(git rev-parse --show-toplevel)/VERSION)
TAG="v${VERSION}.0"

# We have to explicitly delete whitespace since BSD version of wc(1) emits some
# in front of the count.
COMMITS_SINCE=$(git log --oneline ${TAG}..HEAD | wc -l | tr -d ' ')

GIT_HEAD=$(git rev-parse HEAD)
GIT_DIRTY=''
if [ $(git ls-files -m | wc -l) -ne 0 ]; then
    GIT_DIRTY='-dirty'
fi

FINGERPRINT=$(cat $(find ./include ./src -type f | sort) | b2sum -l 160 | cut -d' ' -f1)

MODE=${1:-default}

if test "${MODE}" = '--help'
then
    echo "usage: $0 <style>"
    echo "  <style> must be one of:"
    cat "$0" | grep -P '^\s+[a-z|]+\)  #' | sed 's/)  #/\t/'
    exit 0
fi

case ${MODE} in
    short|default)  # just the version
        echo "${VERSION}.${COMMITS_SINCE}"
        ;;
    full|precise)  # version tagged with the HEAD commit
        echo "${VERSION}.${COMMITS_SINCE}.${GIT_HEAD}${GIT_DIRTY}"
        ;;
    base|release)  # base release (with 0 as patch segment)
        echo "${VERSION}.0"
        ;;
    fingerprint|fp)  # fingerprint of the code
        echo "${FINGERPRINT}"
        ;;
    *)
        exit 1
        ;;
esac
