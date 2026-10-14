#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT HUP INT TERM
cc=${CC:-cc}
cxx=${CXX:-c++}
flags='-g -O1 -fno-omit-frame-pointer -fno-pie -fsanitize=address,undefined'
"$cc" -std=gnu11 $flags -I"$root/J-Link Utils/include/freetype2/ifqueue" \
    -c "$root/J-Link Utils/include/freetype2/ifqueue/lfqueue.c" -o "$build/queue.o"
"$cxx" -std=c++17 $flags -no-pie -pthread -Wall -Wextra \
    -I"$root/tests/transport_stubs" -I"$root/J-Link Utils/include/freetype2" \
    -I"$root/J-Link Utils/include/freetype2/ifqueue" \
    "$root/tests/transport_test.cpp" "$root/J-Link Utils/DataHelper.cpp" \
    "$root/J-Link Utils/include/freetype2/tinyxml2/tinyxml2.cpp" "$build/queue.o" \
    -o "$build/transport_test"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 "$build/transport_test"
