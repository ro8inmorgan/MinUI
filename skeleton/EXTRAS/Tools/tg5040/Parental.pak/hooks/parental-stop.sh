#!/bin/sh
# parental-stop.sh -- tear down the watcher started by parental-gate.sync.sh

[ "$HOOK_TYPE" = "rom" ] || exit 0

PAK="$SDCARD_PATH/Tools/$PLATFORM/Parental.pak"
[ -x "$PAK/parental.elf" ] || exit 0

"$PAK/parental.elf" --stop
