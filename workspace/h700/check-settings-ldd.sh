#!/bin/sh
set -eu

ROOT_DIR="${1:-$(cd "$(dirname "$0")/../.." && pwd)}"
SETTINGS_ELF="${SETTINGS_ELF:-$ROOT_DIR/build/EXTRAS/Tools/h700/Settings.pak/settings.elf}"
H700_LIB_DIR="${H700_LIB_DIR:-$ROOT_DIR/build/SYSTEM/h700/lib}"

if [ ! -f "$SETTINGS_ELF" ]; then
	echo "missing settings.elf: $SETTINGS_ELF" >&2
	exit 1
fi

if [ ! -d "$H700_LIB_DIR" ]; then
	echo "missing h700 lib dir: $H700_LIB_DIR" >&2
	exit 1
fi

export LD_LIBRARY_PATH="$H700_LIB_DIR:${LD_LIBRARY_PATH:-}"
ldd_output="$(ldd "$SETTINGS_ELF" 2>&1)" || {
	echo "$ldd_output"
	exit 1
}

echo "$ldd_output"

if echo "$ldd_output" | grep -q "not found"; then
	echo "settings.elf has unresolved runtime libraries" >&2
	exit 1
fi

if ! echo "$ldd_output" | grep -q "libgio-2.0"; then
	echo "settings.elf is expected to link gio-2.0 on h700" >&2
	exit 1
fi

if ! echo "$ldd_output" | grep -q "libglib-2.0"; then
	echo "settings.elf is expected to link glib-2.0 on h700" >&2
	exit 1
fi
