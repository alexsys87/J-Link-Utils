#!/bin/sh
set -eu
# Build and run a local copy: cloud-mounted compiler IO is unnecessarily slow.
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build=$(mktemp -d /tmp/settings-tests.XXXXXX)
trap 'rm -rf "$build"' EXIT HUP INT TERM
cp "$root/Preferences.h" "$root/Localization.h" "$root/iniConfig.cpp" "$build/"
cp "$root/tests/settings/"*.cpp "$root/tests/settings/Main.h" "$build/"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -pedantic -I"$build" \
    "$build/settings_test.cpp" "$build/other_translation_unit.cpp" "$build/iniConfig.cpp" -o "$build/settings_test"
# Root bypasses filesystem permissions, invalidating permission-denied tests.
if [ "$(id -u)" = 0 ]; then
    chmod 777 "$build"
    runuser -u nobody -- "$build/settings_test"
else
    "$build/settings_test"
fi
