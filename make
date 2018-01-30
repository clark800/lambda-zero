#!/bin/sh
#http://www.gnu.org/software/autoconf/manual/autoconf.html#Portable-Shell

OUT="main"
CC="gcc"
FLAGS="-c -g -Os -std=c99 -pedantic -pedantic-errors -Werror -Wfatal-errors \
-Wall -Wextra -Wshadow -Winit-self -Wwrite-strings -Wconversion \
-Wstrict-prototypes -fno-ident -fno-stack-protector -ffunction-sections \
-fdata-sections -fno-asynchronous-unwind-tables"

echoexec() {
    echo "$@"
    "$@"
}

build_custom() {
    if [[ ! -f "$1" || ! -z `find -L . -name "*.[c|h]" -newer "$1"` ]]; then
        (cd lib/libc && ./make)
        flags="-nostdlib -nostdinc -isystem ./lib/libc/include/ -fno-builtin"
        sources=`find -L . -name "*.c"`
        echoexec $CC $FLAGS $flags $sources
        ldflags="--gc-sections --print-gc-sections --strip-discarded"
        echoexec ld $ldflags lib/libc/sys.o *.o -o "$1"
        strip -s "$1"
        rm *.o
        ls -l "$1"
        return 0
    fi
    return 1
}

build_stdlib() {
    if [[ ! -f "$1" || ! -z \
        `find -L . -name "*.[c|h]" -newer $1 -not -path "./lib/libc/*"` ]]; then
        sources=`find -L . -name "*.c" -not -path "./lib/libc/*"`
        echoexec $CC $FLAGS $sources
        gcc -Wl,--gc-sections,--strip-discarded -o "$1" *.o
        rm *.o
        return 0
    fi
    return 1
}

loc() {
    cloc --quiet --exclude-list-file=.clocignore --by-file $*
}

case "$1" in
    "") build_stdlib "$OUT" || echo "$OUT up-to-date";;
    test) build_stdlib "$OUT"; test/test.sh -x -v;;
    custom) build_custom "$OUT" || echo "$OUT up-to-date";;
    clean) rm -f "$OUT" *.o; rm -f lib/libc/sys.o;;
    cloc) loc --exclude-dir="libc" . | tail -n +3 | cut -c -78;;
    cloc-custom) loc . | tail -n +3 | cut -c -78;;
    *) echo "usage: $0 [{test|custom|clean|cloc|cloc-custom}]"; exit 1;;
esac
