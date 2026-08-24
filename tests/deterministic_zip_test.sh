#!/bin/sh
set -eu

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/input/sub"
printf 'same bytes\n' >"$work/input/sub/file"
printf '#!/bin/sh\n' >"$work/input/run"
chmod 755 "$work/input/run"
SOURCE_DATE_EPOCH=1786810539 python3 ../scripts/deterministic_zip.py --strip-root "$work/one.zip" "$work/input"
touch "$work/input/sub/file"
SOURCE_DATE_EPOCH=1786810539 python3 ../scripts/deterministic_zip.py --strip-root "$work/two.zip" "$work/input"
cmp "$work/one.zip" "$work/two.zip"
