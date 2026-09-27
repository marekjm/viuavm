#!/usr/bin/env sh

set -e

case "${1}" in
    g++*)
        # Why do this if GCC supports the -dumpversion flag?
        # Because GCC will only report the major revision instead of the full
        # version eg, for GCC 16.2.1 it will show "16" instead of "16.2.1".
        ${1} --version | head -n 1 | cut -d' ' -f3
        ;;
    clang++*)
        # Clang's -dumpversion output shows the full version so there is no need
        # for any pipeline shenanigans.
        ${1} -dumpversion
        ;;
    *)
        exit 1
        ;;
esac
