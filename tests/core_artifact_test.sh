#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
temp=$(mktemp -d "${TMPDIR:-/tmp}/nextui-core-artifacts.XXXXXX")
mkdir "$temp/merged"
trap 'rm -rf "$temp"' EXIT HUP INT TERM

cores=$(sed -n 's/^CORES+\?= *//p' "$ROOT/workspace/tg5040/cores/makefile")
count=0
for core in $cores; do
    output=$(cd "$ROOT/workspace/tg5040/cores" && make -n "$core" PLATFORM=tg5040 CROSS_COMPILE=x | awk '/^mv .*\.so \.\/output\/.*\.so$/ { print $NF }')
    [ "$(printf '%s\n' "$output" | sed '/^$/d' | wc -l)" -eq 1 ]
    output=${output#./output/}
    case "$output" in *.so) ;; *) exit 1 ;; esac
    cd "$ROOT/workspace/tg5040/cores"
    make -pn "$core" PLATFORM=tg5040 CROSS_COMPILE=x 2>/dev/null | grep -F "output/$output:" >/dev/null
    mkdir "$temp/$core"
    : >"$temp/$core/$output"
    cp "$temp/$core/$output" "$temp/merged/"
    count=$((count + 1))
done

[ "$count" -eq 28 ]
[ "$(find "$temp/merged" -type f -name '*.so' | wc -l)" -eq 28 ]
for workflow in "$ROOT/.github/workflows/ci.yaml" "$ROOT/.github/workflows/release.yaml"; do
    grep -F 'set -- workspace/$PLATFORM/cores/output/*.so' "$workflow" >/dev/null
    grep -F 'path: workspace/${{ matrix.toolchain }}/cores/output/' "$workflow" >/dev/null
    grep -F 'merge-multiple: true' "$workflow" >/dev/null
done

echo "core artifact mappings: $count passed"
