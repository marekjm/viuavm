#!/usr/bin/env sh

set -e

VIUA_DIR=~/.local/lib/viua

function main {
    TOOL=${1}
    case ${TOOL} in
        asm|dis|vm|readelf|repl)
            exec ${VIUA_DIR}/viua-core/${TOOL} ${@:2}
            ;;
        --version)
            exec ${VIUA_DIR}/viua-core/vm ${@:1}
            ;;
        '')
            exec ${VIUA_DIR}/viua-core/vm --version
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
