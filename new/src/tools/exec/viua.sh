#!/usr/bin/env sh

set -e

TOOL=${1}
CORE_DIR=~/.local/lib/viua/core

case ${TOOL} in
    asm|dis|vm|readelf)
        exec ${CORE_DIR}/bin/${TOOL} ${@:2}
        ;;
    --version)
        exec ${CORE_DIR}/bin/vm ${@:1}
        ;;
    --*)
        2>&1 echo "viua: error: unknown option \`${TOOL}'"
        exit 1
        ;;
    *)
        2>&1 echo "viua: error: \`${TOOL}' is not a part of the toolchain"
        exit 1
        ;;
esac
