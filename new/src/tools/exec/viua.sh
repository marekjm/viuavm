#!/bin/sh

set -e

PREFIX=$(realpath $(dirname $(realpath "${0}"))/../../../build)

LIBDIR=${PREFIX}/lib
EXEDIR=${PREFIX}/libexec/viua
BINDIR=${PREFIX}/bin

viua_invoke_tool () {
    exec ${EXEDIR}/${@}
}

main () {
    TOOL=${1}
    case ${TOOL} in
        asm|dis|vm|readelf|repl)
            viua_invoke_tool ${@}
            ;;
        help)
            SUBJECT=${2:-viua}
            exec man viua-${SUBJECT}
            ;;
        --help|'')
            exec man viua-viua
            ;;
        --version)
            exec ${EXEDIR}/vm ${@} | sed 's/ vm//'
            ;;
        --built-with)
            exec ${EXEDIR}/vm ${@} | sed 's/ vm//'
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

main ${@}
