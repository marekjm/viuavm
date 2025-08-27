#!/usr/bin/env bash

set -e

default_impl=${VIUAVM_IO_IMPL:-auto}
impl=${1:-${default_impl}}

if [[ ${impl} == "auto" ]]; then
    case $(uname -s) in
        Linux)
            impl=io_uring
            ;;
        FreeBSD|NetBSD)
            impl=classic
            ;;
        *)
            exit 1
            ;;
    esac
fi

echo "${impl}"
