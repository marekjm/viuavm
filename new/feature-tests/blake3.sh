#!/usr/bin/env bash

set -e

answer=${1:-auto}

if [[ ${answer} == "auto" ]]; then
    case $(uname -s) in
        Linux)
            answer=yes
            ;;
        FreeBSD)
            answer=
            ;;
        *)
            exit 1
            ;;
    esac
fi

echo "${answer}"
