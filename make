#!/bin/bash
# http://www.gnu.org/software/autoconf/manual/autoconf.html#Portable-Shell

OUT="main"
CC="gcc"

# stack-protector should be off by default, but some linux distributions
# patch gcc to turn it on by default, so we explicitly disable it here; it
# will cause the nostdlib build to fail if it is enabled

# no-asynchronous-unwind-tables decreases the executable size and there seems
# to be no downside because the information will still be included when
# compiling with "-g" https://stackoverflow.com/a/26302715

# no-ident removes a small unneeded comment from the executable

CC_FLAGS="-c -std=c99 -pedantic -pedantic-errors \
-Werror -Wfatal-errors -Wall -Wextra -Wshadow -Winit-self -Wwrite-strings \
-Wconversion -Wcast-qual -Wstrict-prototypes \
-fno-ident -fno-stack-protector -fno-asynchronous-unwind-tables"

# gcc sets --hash-style to a non-default setting when calling ld, which
# increases the executable size; here we set it back to the default

LINK_FLAGS="-Wl,--hash-style=sysv,--gc-sections,--strip-discarded,--print-gc-sections"
SOURCES=`find -L . -name "*.c" -not -path "./lib/libc/*"`
OBJECTS="*.o"

echoexec() {
    echo "$@"
    "$@"
}

clean() {
    rm -f $OBJECTS
    rm -f "$OUT"
}

is_up_to_date() {
    local output="$1"
    [[ -f "$output" && -z `find -L . -name "*.[c|h]" -newer "$output"` ]]
}

build_libc() {
    (cd lib/libc && ./make)
}

build() {
    if ! is_up_to_date "$OUT"; then
        echoexec $CC $CC_FLAGS $SOURCES &&
        echoexec $CC $LINK_FLAGS -o "$OUT" $OBJECTS &&
        rm -f $OBJECTS &&
        echo &&
        echo "BUILD SUCCESSFUL:" &&
        ls -gG "$OUT" || (rm -f $OBJECTS ; false)
        return
    fi
    echo "$OUT up-to-date"
}

config_fast() {
    CC_FLAGS="-DNDEBUG -O3 -flto $CC_FLAGS"
    LINK_FLAGS="-flto -Wl,--strip-all $LINK_FLAGS"
}

config_debug() {
    CC_FLAGS="-g -Og $CC_FLAGS"
}

config_custom() {
    custom="-nostdlib -nostdinc -isystem ./lib/libc/include/ -fno-builtin"
    CC_FLAGS="$custom $CC_FLAGS"
    LINK_FLAGS="-nostdlib $LINK_FLAGS"
    SOURCES=`find -L . -name "*.c"`
    OBJECTS="*.o lib/libc/sys.o"
}

config_small() {
    config_custom
    CC_FLAGS="-s -Os -flto $CC_FLAGS"
    LINK_FLAGS="-flto -Wl,--strip-all $LINK_FLAGS"
}

config_unused() {
    config_custom
    CC_FLAGS="-O0 -fdata-sections -ffunction-sections $CC_FLAGS"
}

loc() {
    cloc --quiet --exclude-list-file=.clocignore --by-file $*
}

case "$1" in
    "") config_fast && build;;
    fast) config_fast && build;;
    small) config_small && build_libc && build;;
    unused) clean && config_unused && build_libc && build;;
    test) config_debug && build && test/test.sh;;
    clean) clean;;
    cloc) loc --exclude-dir="libc" . | tail -n +3 | cut -c -78;;
    cloc-all) loc . | tail -n +3 | cut -c -78;;
    *) echo "usage: $0 [{fast|test|small|unused|clean|cloc|cloc-all}]"; exit 1;;
esac
