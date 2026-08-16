#!/bin/sh
# parental-gate.sync.sh -- refuse a rom launch once the daily budget is spent.
#
# Must stay named *.sync.sh: only synchronous pre-launch hooks get a veto,
# and a non-zero exit is what cancels the launch.

[ "$HOOK_TYPE" = "rom" ] || exit 0

# Emulators the owner exempted in the settings screen. HOOK_EMU_PATH points at
# the emulator's launch.sh, so the pak name is its parent directory -- basename
# alone would be "launch.sh" for every emulator alike.
#
# This is what lets tool shortcuts through: they are launched by the bridge
# emulator of the Shortcuts pak, which makes them look like roms to the launcher.
# Deny by default, so a pak that is not named here stays blocked.
EMU_PAK=$(basename "$(dirname "$HOOK_EMU_PATH")")
EXEMPT=$(sed -n 's/^exempt=//p' "$SHARED_USERDATA_PATH/parental.txt" 2>/dev/null)
case ",$EXEMPT," in
	*",$EMU_PAK,"*) exit 0 ;;
esac

PAK="$SDCARD_PATH/Tools/$PLATFORM/Parental.pak"
[ -x "$PAK/parental.elf" ] || exit 0

if "$PAK/parental.elf" --gate; then
	# budget left: watch the session so it can be stopped on time
	"$PAK/parental.elf" --watch &
	exit 0
fi

# nextui already opened a play_activity row before handing off to us, close it
# so the refused launch doesn't count as time played
gametimectl.elf stop_all

# --image is mandatory, show2.elf prints its usage and draws nothing without it
show2.elf --mode=simple --image="$SDCARD_PATH/.system/res/logo.png" --text="No play time left today" --timeout=3

exit 1
