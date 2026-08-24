#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
TEST_ROOT=$(mktemp -d)
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM
cp "$ROOT/makefile" "$TEST_ROOT/Makefile"
mkdir -p "$TEST_ROOT/build/PAYLOAD/.system" "$TEST_ROOT/build/PAYLOAD/.tmp_update" "$TEST_ROOT/build/PAYLOAD/Tools"
printf system >"$TEST_ROOT/build/PAYLOAD/.system/file"
printf update >"$TEST_ROOT/build/PAYLOAD/.tmp_update/file"
printf tools >"$TEST_ROOT/build/PAYLOAD/Tools/file"
manifest=$(cd "$TEST_ROOT" && make -n -f Makefile package 2>/dev/null | grep '^cd ./build/PAYLOAD && python3 ' | head -n 1)
[ -n "$manifest" ]
(cd "$TEST_ROOT" && sh -c "$manifest")
(cd "$TEST_ROOT/build/PAYLOAD" && sha256sum -c SHA256SUMS)
[ "$(wc -l <"$TEST_ROOT/build/PAYLOAD/SHA256SUMS")" -eq 3 ]
