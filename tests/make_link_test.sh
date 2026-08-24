#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
args='PLATFORM=tg5040 CROSS_COMPILE=x CC=true CXX=true PREFIX=/tmp/nextui-prefix PREFIX_LOCAL=/tmp/nextui-prefix-local'

for target in libgametimedb libbatmondb; do
    output=$(cd "$ROOT/workspace/all/$target" && make -n build $args)
    printf '%s\n' "$output" | grep -F '"path_helpers.o"' >/dev/null
done

output=$(cd "$ROOT/workspace/all/settings" && make -n all $args)
printf '%s\n' "$output" | grep -F 'mv utils.o path_helpers.o' >/dev/null
printf '%s\n' "$output" | grep -F 'build/tg5040/path_helpers.o' >/dev/null
