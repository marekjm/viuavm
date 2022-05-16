#!/usr/bin/env sh

set -e

VIUA_DIR=~/.local/lib/viua

function show_help {
    local WHAT_FOR=${1:-overview}
    exec man 1 viua-${WHAT_FOR}
}

function main {
    TOOL=${1}
    case ${TOOL} in
        asm|dis|vm|readelf|repl)
            exec ${VIUA_DIR}/viua-core/${TOOL} ${@:2}
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
            exec ${VIUA_DIR}/viua-core/vm ${@:1}
            ;;
        -*)
            2>&1 echo "viua: error: unknown option \`${TOOL}'"
            exit 1
            ;;
        *)
            2>&1 echo "viua: error: \`${TOOL}' is not a part of the toolchain"
            exit 1
            ;;
    esac
}

main "${@}"
