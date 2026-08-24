#!/bin/sh

# Create repo-local, suffix-less Homebrew GCC tool links. Never use sudo.
ROOT=$(CDPATH='' cd "$(dirname "$0")/../.." && pwd) || exit 1
TARGET_DIR="$ROOT/workspace/desktop/.toolchain"
BREW=$(command -v brew) || {
	echo "Homebrew is required" >&2
	exit 1
}
GCC_BASE=$("$BREW" --prefix gcc 2>/dev/null) || {
	echo "Homebrew gcc is required" >&2
	exit 1
}
GCC_BIN_DIR=$GCC_BASE/bin
[ -d "$GCC_BIN_DIR" ] || {
	echo "Homebrew gcc is required" >&2
	exit 1
}

mkdir -p "$TARGET_DIR" || exit 1
for name in gcc g++ gcc-ar; do
	set -- "$GCC_BIN_DIR"/"$name"-[0-9]*
	[ "$#" -eq 1 ] && [ -x "$1" ] || {
		echo "Expected exactly one executable $name in $GCC_BIN_DIR" >&2
		exit 1
	}
	case "$name" in gcc-ar) link="ar" ;; *) link="$name" ;; esac
	ln -sfn "$1" "$TARGET_DIR/$link" || exit 1
done

echo "Created $TARGET_DIR (use make PLATFORM=desktop ...)."
