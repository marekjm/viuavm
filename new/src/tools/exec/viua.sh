#!/usr/bin/env sh

set -e

function show_help {
    local WHAT_FOR=${1:-viua}
    exec man viua-${WHAT_FOR}
}

PREFIX=/usr
LIBDIR=${PREFIX}/lib
EXEDIR=${PREFIX}/libexec/viua
BINDIR=${PREFIX}/bin

function main {
    TOOL=${1}
    case ${TOOL} in
        asm|dis|vm|readelf|repl)
            set -x
            exec ${EXEDIR}/${TOOL} "${@:2}"
            ;;
        help)
            show_help "${@:2}"
            exit 0
            ;;
        --help|'')
            show_help
            exit 0
            ;;
        --version)
            exec ${EXEDIR}/vm ${@:1} | sed 's/ vm//'
            ;;
        --prefix)
            echo "PREFIX=${PREFIX}"
            ;;
        -*)
            2>&1 echo "viua: error: unknown option \`${TOOL}'"
            exit 1
            ;;
        *)
            2>&1 echo "viua: error: not a part of the viua toolchain: \`${TOOL}'"
            exit 1
            ;;
    esac
}

main "${@}"
