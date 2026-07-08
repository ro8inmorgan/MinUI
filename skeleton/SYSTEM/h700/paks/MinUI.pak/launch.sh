#!/bin/sh
# MinUI.pak for Anbernic RG XX / Allwinner H700

export PLATFORM="h700"
REAL_SDCARD_PATH="/mnt/sdcard"
COMPAT_SDCARD_PATH="/mnt/SDCARD"

mkdir -p "$REAL_SDCARD_PATH"
if ! mountpoint -q "$REAL_SDCARD_PATH"; then
	mount -t vfat -o rw,utf8,noatime /dev/mmcblk1p1 "$REAL_SDCARD_PATH" 2>/dev/null || true
fi

if [ ! -e "$COMPAT_SDCARD_PATH" ]; then
	ln -s "$REAL_SDCARD_PATH" "$COMPAT_SDCARD_PATH" 2>/dev/null || true
fi

export SDCARD_PATH="$COMPAT_SDCARD_PATH"
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
export HOME="$USERDATA_PATH"
DEBUG_KEEP_NETWORK_PATH="$USERDATA_PATH/debug-keep-network"
LAUNCH_LOG="$LOGS_PATH/launch.txt"

export PATH="$SYSTEM_PATH/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:/usr/lib:/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH"
export SDL_VIDEODRIVER="mali"
export SDL_AUDIODRIVER="alsa"

if [ -f "/tmp/poweroff" ]; then
	poweroff
	exit 0
fi
if [ -f "/tmp/reboot" ]; then
	reboot
	exit 0
fi

mkdir -p "$BIOS_PATH" "$ROMS_PATH" "$SAVES_PATH" "$CHEATS_PATH"
mkdir -p "$USERDATA_PATH" "$LOGS_PATH" "$HOOKS_PATH" "$SHARED_USERDATA_PATH/.minui"
echo "launch: starting $(date)" > "$LAUNCH_LOG"

export RGXX_MODEL="$(strings /mnt/vendor/bin/dmenu.bin 2>/dev/null | grep -m1 '^RG')"
case "$RGXX_MODEL" in
	RG28xx) export DEVICE="rg28xx" ;;
	RG34xx*|RG34XX*) export DEVICE="rg34xx" ;;
	RGcubexx) export DEVICE="cube" ;;
	*) export DEVICE="rg40xx" ;;
esac
export IS_NEXT="yes"

if [ -f /mnt/vendor/muos1.ini ] || [ -f /mnt/vendor/muos2.ini ]; then
	echo "stockmod muOS override files are present; stock boot target must be selected for TF1 dmenu.bin autoboot." > "$LOGS_PATH/stockmod-warning.txt"
fi

killall brightCtrl.bin cexpert 2>/dev/null || true
if [ -f "$DEBUG_KEEP_NETWORK_PATH" ]; then
	echo "launch: keeping stock network services for debug" >> "$LAUNCH_LOG"
else
	systemctl stop NetworkManager 2>/dev/null || true
fi
loginctl show-logind >/dev/null 2>&1 && mkdir -p /run/systemd/system 2>/dev/null || true

if [ -f "$SYSTEM_PATH/dat/dmenu.bin" ] && [ -d /mnt/mmc ]; then
	if ! cmp -s "$SYSTEM_PATH/dat/dmenu.bin" /mnt/mmc/dmenu.bin 2>/dev/null; then
		cp "$SYSTEM_PATH/dat/dmenu.bin" /mnt/mmc/dmenu.bin 2>/dev/null || true
		sync
	fi
fi

rm -rf "$SDCARD_PATH/.shadercache"
echo 0 > /sys/class/power_supply/axp2202-battery/work_led 2>/dev/null || true

sh "$SYSTEM_PATH/bin/governor.sh" "auto"

keymon.elf > "$LOGS_PATH/keymon.txt" 2>&1 &
batmon.elf > "$LOGS_PATH/batmon.txt" 2>&1 &

rm -f "$USERDATA_PATH/.asoundrc"
audiomon.elf > "$LOGS_PATH/audiomon.txt" 2>&1 &

bluetoothon=$(nextval.elf bluetooth | sed -n 's/.*"bluetooth": \([0-9]*\).*/\1/p')
if [ "$bluetoothon" = "0" ]; then
	"$SYSTEM_PATH/etc/bluetooth/bt_init.sh" stop > /dev/null 2>&1 &
else
	"$SYSTEM_PATH/etc/bluetooth/bt_init.sh" start > /dev/null 2>&1 &
fi

wifion=$(nextval.elf wifi | sed -n 's/.*"wifi": \([0-9]*\).*/\1/p')
if [ -f "$DEBUG_KEEP_NETWORK_PATH" ]; then
	echo "launch: skipping NextUI wifi init for debug" >> "$LAUNCH_LOG"
elif [ "$wifion" = "0" ]; then
	"$SYSTEM_PATH/etc/wifi/wifi_init.sh" stop > /dev/null 2>&1 &
else
	"$SYSTEM_PATH/etc/wifi/wifi_init.sh" start > /dev/null 2>&1 &
fi

AUTO_PATH="$USERDATA_PATH/auto.sh"
if [ -f "$AUTO_PATH" ]; then
	"$AUTO_PATH"
fi
"$SYSTEM_PATH/bin/run_hooks.sh" boot.d

cd "$(dirname "$0")"
killall -9 show2.elf > /dev/null 2>&1 || true

parse_hook_cmd() {
	HOOK_CMD="$1"
	HOOK_EMU_PATH=$(echo "$HOOK_CMD" | sed "s/^'\\([^']*\\)'.*/\\1/")
	_remainder=$(echo "$HOOK_CMD" | sed "s/^'[^']*'//")
	if echo "$_remainder" | grep -q "'"; then
		HOOK_TYPE="rom"
		HOOK_ROM_PATH=$(echo "$_remainder" | sed "s/.*'\\([^']*\\)'.*/\\1/")
	else
		HOOK_TYPE="pak"
		HOOK_ROM_PATH=""
	fi
	[ -f /tmp/last.txt ] && HOOK_LAST=$(cat /tmp/last.txt) || HOOK_LAST=""
	export HOOK_CMD HOOK_EMU_PATH HOOK_TYPE HOOK_ROM_PATH HOOK_LAST
}

EXEC_PATH="/tmp/nextui_exec"
NEXT_PATH="/tmp/next"
touch "$EXEC_PATH" && sync
while [ -f "$EXEC_PATH" ]; do
	echo "launch: starting nextui.elf $(date)" >> "$LAUNCH_LOG"
	nextui.elf > "$LOGS_PATH/nextui.txt" 2>&1
	echo "launch: nextui.elf exited $? $(date)" >> "$LAUNCH_LOG"
	sh "$SYSTEM_PATH/bin/governor.sh" "performance"

	if [ -f "$NEXT_PATH" ]; then
		CMD="$(cat "$NEXT_PATH")"
		parse_hook_cmd "$CMD"
		"$SYSTEM_PATH/bin/run_hooks.sh" pre-launch.d
		eval "$CMD"
		"$SYSTEM_PATH/bin/run_hooks.sh" post-launch.d
		rm -f "$NEXT_PATH"
		sh "$SYSTEM_PATH/bin/governor.sh" "performance"
	fi

	if [ -f "/tmp/poweroff" ]; then
		poweroff
		exit 0
	fi
	if [ -f "/tmp/reboot" ]; then
		reboot
		exit 0
	fi
done

poweroff
