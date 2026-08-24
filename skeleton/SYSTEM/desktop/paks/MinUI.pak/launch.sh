#!/bin/sh
# MinUI.pak

export PLATFORM="desktop"
# I put some thinking into what path to use, this is the only one that ticks all the boxes:
# - writable by normal user
# - not likely to be accidentally deleted by user
# - not in home directory to avoid having to expand ~ or $HOME in #defines or scripts
# - clearly for temporary/debug use only
# - works on both macOS and Linux
export SDCARD_PATH="/var/tmp/nextui/sdcard"
export BIOS_PATH="$SDCARD_PATH/Bios"
export ROMS_PATH="$SDCARD_PATH/Roms"
export SAVES_PATH="$SDCARD_PATH/Saves"
export CHEATS_PATH="$SDCARD_PATH/Cheats"
export SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"
export CORES_PATH="$SYSTEM_PATH/cores"
export USERDATA_PATH="$SDCARD_PATH/.userdata/$PLATFORM"
export SHARED_USERDATA_PATH="$SDCARD_PATH/.userdata/shared"
export LOGS_PATH="$USERDATA_PATH/logs"
export HOOKS_PATH="$USERDATA_PATH/.hooks"
export DATETIME_PATH="$SHARED_USERDATA_PATH/datetime.txt"

mkdir -p "$BIOS_PATH"
mkdir -p "$ROMS_PATH"
mkdir -p "$SAVES_PATH"
mkdir -p "$CHEATS_PATH"
mkdir -p "$USERDATA_PATH"
mkdir -p "$LOGS_PATH"
mkdir -p "$HOOKS_PATH"
mkdir -p "$SHARED_USERDATA_PATH/.minui"

export IS_NEXT="yes"

#######################################

export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DYLD_LIBRARY_PATH
export PATH=$SYSTEM_PATH/bin:$PATH

#batmon.elf & # logging intentionally disabled

#######################################

AUTO_PATH=$USERDATA_PATH/auto.sh
if [ -f "$AUTO_PATH" ]; then
	"$AUTO_PATH"
fi

# Composable boot hooks (run after auto.sh for backward compatibility)
"$SYSTEM_PATH/bin/run_hooks.sh" boot.d

cd "$(dirname "$0")" || exit 1

parse_launch_record() {
	[ -f "$NEXT_PATH" ] && [ ! -L "$NEXT_PATH" ] || return 1
	[ "$(wc -c <"$NEXT_PATH")" -le 8192 ] 2>/dev/null || return 1
	exec 3<"$NEXT_PATH" || return 1
	IFS= read -r magic <&3 && IFS= read -r count <&3 || {
		exec 3<&-
		return 1
	}
	[ "$magic" = "NEXTUI_ARGV_V1" ] || {
		exec 3<&-
		return 1
	}
	case "$count" in 1 | 2) ;; *)
		exec 3<&-
		return 1
		;;
	esac
	IFS= read -r length <&3 || {
		exec 3<&-
		return 1
	}
	case "$length" in '' | *[!0-9]*)
		exec 3<&-
		return 1
		;;
	esac
	[ "$length" -gt 0 ] && [ "$length" -lt 4096 ] || {
		exec 3<&-
		return 1
	}
	IFS= read -r LAUNCH_PROGRAM <&3 || {
		exec 3<&-
		return 1
	}
	[ "$(LC_ALL=C printf '%s' "$LAUNCH_PROGRAM" | wc -c)" -eq "$length" ] || {
		exec 3<&-
		return 1
	}
	LAUNCH_ARGUMENT=""
	if [ "$count" = 2 ]; then
		IFS= read -r length <&3 || {
			exec 3<&-
			return 1
		}
		case "$length" in '' | *[!0-9]*)
			exec 3<&-
			return 1
			;;
		esac
		[ "$length" -gt 0 ] && [ "$length" -lt 4096 ] || {
			exec 3<&-
			return 1
		}
		IFS= read -r LAUNCH_ARGUMENT <&3 || {
			exec 3<&-
			return 1
		}
		[ "$(LC_ALL=C printf '%s' "$LAUNCH_ARGUMENT" | wc -c)" -eq "$length" ] || {
			exec 3<&-
			return 1
		}
	fi
	IFS= read -r extra <&3 && {
		: "$extra"
		exec 3<&-
		return 1
	}
	exec 3<&-
	return 0
}

prepare_launch_hooks() {
	HOOK_EMU_PATH="$LAUNCH_PROGRAM"
	HOOK_ROM_PATH="$LAUNCH_ARGUMENT"
	if [ -n "$LAUNCH_ARGUMENT" ]; then HOOK_TYPE="rom"; else HOOK_TYPE="pak"; fi
	HOOK_CMD="$LAUNCH_PROGRAM${LAUNCH_ARGUMENT:+ $LAUNCH_ARGUMENT}"
	[ -f /tmp/last.txt ] && HOOK_LAST=$(cat /tmp/last.txt) || HOOK_LAST=""
	export HOOK_CMD HOOK_EMU_PATH HOOK_TYPE HOOK_ROM_PATH HOOK_LAST
}

RUNTIME_PATH="/tmp/nextui-runtime"
ACTIVE_ROM_PATH="$RUNTIME_PATH/active.rom"
export NEXTUI_RUNTIME_PATH="$RUNTIME_PATH"
NEXT_PATH="$RUNTIME_PATH/launch.argv"
mkdir -p "$RUNTIME_PATH" && chmod 700 "$RUNTIME_PATH" || exit 1
nextui.elf >"$LOGS_PATH/nextui.txt" 2>&1
if parse_launch_record; then
	prepare_launch_hooks
	rm -f "$NEXT_PATH"
	"$SYSTEM_PATH/bin/run_hooks.sh" pre-launch.d
	if [ "$HOOK_TYPE" = "rom" ]; then
		(
			umask 077
			printf '%s\n' "$LAUNCH_ARGUMENT" >"$ACTIVE_ROM_PATH"
		) || exit 1
		gametimectl.elf start "$LAUNCH_ARGUMENT"
		"$LAUNCH_PROGRAM" "$LAUNCH_ARGUMENT"
		rm -f "$ACTIVE_ROM_PATH"
	else
		"$LAUNCH_PROGRAM"
	fi
	"$SYSTEM_PATH/bin/run_hooks.sh" post-launch.d
else
	printf '%s\n' "NextUI: invalid launch record: $NEXT_PATH" >>"$LOGS_PATH/nextui.txt"
fi
