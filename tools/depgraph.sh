#!/bin/sh

get_includes() {
    dirname="$(dirname "$1")"
    filename="$(basename "$1" | cut -d'.' -f 1)"
    sed -n 's/^#include[[:space:]]*"\([^"]*\)"/\1/p' "$1" |
    while read -r path; do
        if [ -r "$dirname/$path" ]; then
            echo "${path##*/}"
        fi
    done |
    cut -d'.' -f 1 | grep -v -x "$filename" |
    sed -e "s|^|    $filename -> |" -e 's|$|;|'
}

build_graph() {
    root="$1"
    echo "digraph {"
    echo "    size=6;"
    echo "    ranksep=1.2;"
    echo "    nodesep=0.8;"
    echo "    mclimit=100;"
    echo
    find "$root" -name "*.c" |
    while read -r path; do
        get_includes "$path"
    done
    echo "}"
}

build_graph "$@"
