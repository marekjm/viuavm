#!/bin/sh

set -e

# See https://www.gnu.org/prep/standards/html_node/Directory-Variables.html for
# more information.
prefix=$(realpath $(dirname $(realpath "${0}"))/../../build)
libdir=${prefix}/lib
libexecdir=${prefix}/libexec
bindir=${prefix}/bin

# See the explanation for "exedir" in the Makefile in Viua's repository for more
# information.
exedir=${libexecdir}/viua


viua_invoke_tool () {
    exec ${exedir}/"${@}"
}

main () {
    local tool=${1}
    case ${tool} in
        asm|dis|vm|readelf|repl)
            viua_invoke_tool "${@}"
            ;;
        help)
            local subject=${2:-viua}
            exec man viua-${subject}
            ;;
        --help|'')
            exec man viua-viua
            ;;
        --version)
            exec ${exedir}/vm ${@} | sed 's/ vm//'
            ;;
        --built-with)
            exec ${exedir}/vm ${@} | sed 's/ vm//'
            ;;
        --prefix)
            echo "${prefix}"
            ;;
        -*)
            2>&1 echo "viua: error: unknown option \`${tool}'"
            exit 1
            ;;
        *)
            2>&1 echo "viua: error: not a part of the viua toolchain: \`${tool}'"
            exit 1
            ;;
    esac
}

main "${@}"
