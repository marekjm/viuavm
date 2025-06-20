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
    local tool="${1}"
    if [ ! -x "${exedir}/${tool}" ]; then
        2>&1 echo "viua: error: \`${tool}' is not part of the toolchain"
        exit 1
    fi
    exec "${exedir}/${@}"
}

viua_show_help () {
    local subject="${1:-viua}"
    exec man --manpath ${prefix}/share/man viua-${subject}
}

main () {
    local tool=${1}
    case ${tool} in
        --help|help|'')
            viua_show_help "${2}"
            ;;
        --version)
            exec "${exedir}/vm" "${@}" | sed 's/^vm //' | tr -d '()'
            ;;
        --built-with)
            exec "${exedir}/vm" "${@}" | sed 's/ vm//'
            ;;
        --prefix)
            echo "${prefix}"
            ;;
        -*)
            2>&1 echo "viua: error: unknown option \`${tool}'"
            exit 1
            ;;
        *)
            viua_invoke_tool "${@}"
            ;;
    esac
}

main "${@}"
