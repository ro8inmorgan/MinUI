#!/bin/sh
# MinUI.pak for Anbernic RG XX / Allwinner H700

export PLATFORM="h700"
REAL_SDCARD_PATH="/mnt/sdcard"
COMPAT_SDCARD_PATH="/mnt/SDCARD"

mkdir -p "$REAL_SDCARD_PATH"
if ! mountpoint -q "$REAL_SDCARD_PATH"; then
	mount -t vfat -o rw,utf8,noatime /dev/mmcblk1p1 "$REAL_SDCARD_PATH" 2>/dev/null || true
fi

if ! mountpoint -q "$COMPAT_SDCARD_PATH" && [ ! -L "$COMPAT_SDCARD_PATH" ]; then
	if [ -e "$COMPAT_SDCARD_PATH" ]; then
		mount --bind "$REAL_SDCARD_PATH" "$COMPAT_SDCARD_PATH" 2>/dev/null || true
	else
		ln -s "$REAL_SDCARD_PATH" "$COMPAT_SDCARD_PATH" 2>/dev/null || true
	fi
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
LAUNCH_LOG="$LOGS_PATH/launch.txt"

export PATH="$SYSTEM_PATH/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:/usr/lib:/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH"
export CURL_CA_BUNDLE="$SYSTEM_PATH/etc/ssl/certs/ca-certificates.crt"
export SDL_VIDEODRIVER="mali"
export SDL_AUDIODRIVER="alsa"
export SDL_JOYSTICK_DISABLE_UDEV=1
export SDL_HIDAPI_JOYSTICK_DISABLE_UDEV=1

for egl_path in /usr/lib/libEGL.so /usr/lib/libEGL.so.1 /usr/lib/libEGL.so.1.4.0 /usr/lib/aarch64-linux-gnu/libEGL.so /usr/lib/aarch64-linux-gnu/libEGL.so.1; do
	if [ -e "$egl_path" ]; then
		export SDL_VIDEO_EGL_DRIVER="$egl_path"
		break
	fi
done
for gles_path in /usr/lib/libGLESv2.so /usr/lib/libGLESv2.so.2 /usr/lib/libGLESv2.so.2.1.0 /usr/lib/aarch64-linux-gnu/libGLESv2.so /usr/lib/aarch64-linux-gnu/libGLESv2.so.2; do
	if [ -e "$gles_path" ]; then
		export SDL_VIDEO_GL_DRIVER="$gles_path"
		break
	fi
done

log_runtime_state() {
	echo "launch: runtime state $(date)" >> "$LAUNCH_LOG"
	echo "launch: DEVICE=$DEVICE RGXX_MODEL=$RGXX_MODEL" >> "$LAUNCH_LOG"
	echo "launch: LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> "$LAUNCH_LOG"
	ldd "$SYSTEM_PATH/bin/nextui.elf" >> "$LAUNCH_LOG" 2>&1 || true
	ldd "$SYSTEM_PATH/lib/libSDL2-2.0.so.0" >> "$LAUNCH_LOG" 2>&1 || true
	ls -l /usr/lib/libEGL.so* /usr/lib/libGLESv2.so* /usr/lib/aarch64-linux-gnu/libEGL.so* /usr/lib/aarch64-linux-gnu/libGLESv2.so* >> "$LAUNCH_LOG" 2>&1 || true
}

ensure_system_dbus() {
	[ -S /run/dbus/system_bus_socket ] && return 0
	command -v dbus-daemon >/dev/null 2>&1 || return 1

	mkdir -p /run/dbus
	if [ ! -s /run/machine-id ] && command -v dbus-uuidgen >/dev/null 2>&1; then
		dbus-uuidgen > /run/machine-id 2>/dev/null || true
	fi
	dbus-daemon --system >/dev/null 2>&1 || true

	dbus_tries=0
	while [ ! -S /run/dbus/system_bus_socket ] && [ "$dbus_tries" -lt 20 ]; do
		sleep 0.1 2>/dev/null || sleep 1
		dbus_tries=$((dbus_tries + 1))
	done
	[ -S /run/dbus/system_bus_socket ]
}

if [ -f "/tmp/poweroff" ]; then
	poweroff_next || poweroff
	exit 0
fi
if [ -f "/tmp/reboot" ]; then
	reboot_next || reboot
	exit 0
fi

mkdir -p "$BIOS_PATH" "$ROMS_PATH" "$SAVES_PATH" "$CHEATS_PATH"
mkdir -p "$USERDATA_PATH" "$LOGS_PATH" "$HOOKS_PATH" "$SHARED_USERDATA_PATH/.minui"
echo "launch: starting $(date)" > "$LAUNCH_LOG"

export RGXX_MODEL="$(strings /mnt/vendor/bin/dmenu.bin 2>/dev/null | grep -m1 '^RG')"
case "$RGXX_MODEL" in
	RG28xx) export DEVICE="rg28xx" ;;
	# The RG SP reports a bare "RGSP" -- no "xx" -- so it matched none of the
	# family globs and fell through to the rg40xx fallback, which drew the UI
	# 640x480 on its 720x480 panel. It shares the RG34XXSP panel exactly (same
	# lcd_driver_name rg34xxsp_v1, DTB fingerprint 26-820-536) but has no analog
	# sticks, so it gets its own DEVICE rather than an rg34xx alias.
	RGSP) export DEVICE="rgsp" ;;
	RG34xx*|RG34XX*) export DEVICE="rg34xx" ;;
	RG35xx*|RG35XX*) export DEVICE="rg35xx" ;;
	RG40xx*|RG40XX*) export DEVICE="rg40xx" ;;
	RGcubexx) export DEVICE="cube" ;;
	*) export DEVICE="rg40xx" ;;
esac
if [ "$DEVICE" = "rg28xx" ]; then
	export SDL_ROTATION="${SDL_ROTATION:-1}"
fi
export IS_NEXT="yes"
log_runtime_state

if [ -f /mnt/vendor/muos1.ini ] || [ -f /mnt/vendor/muos2.ini ]; then
	echo "stockmod muOS override files are present; stock boot target must be selected for TF1 dmenu.bin autoboot." > "$LOGS_PATH/stockmod-warning.txt"
fi

killall brightCtrl.bin cexpert 2>/dev/null || true
systemctl stop NetworkManager 2>/dev/null || true
if loginctl show-logind >/dev/null 2>&1; then
	mkdir -p /run/systemd/logind.conf.d /run/systemd/system 2>/dev/null || true
	cat > /run/systemd/logind.conf.d/nextui-h700.conf << EOF
[Login]
HandlePowerKey=ignore
HandlePowerKeyLongPress=ignore
EOF
	systemctl restart systemd-logind 2>/dev/null || true
fi

if [ -f "$SYSTEM_PATH/dat/dmenu.bin" ] && mountpoint -q /mnt/mmc; then
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
ensure_system_dbus || echo "launch: system D-Bus unavailable; audio device monitoring may be disabled" >> "$LAUNCH_LOG"
audiomon.elf > "$LOGS_PATH/audiomon.txt" 2>&1 &

bluetoothon=$(nextval.elf bluetooth | sed -n 's/.*"bluetooth": \([0-9]*\).*/\1/p')
if [ "$bluetoothon" = "0" ]; then
	"$SYSTEM_PATH/etc/bluetooth/bt_init.sh" stop > /dev/null 2>&1 &
else
	"$SYSTEM_PATH/etc/bluetooth/bt_init.sh" start > /dev/null 2>&1 &
fi

wifion=$(nextval.elf wifi | sed -n 's/.*"wifi": \([0-9]*\).*/\1/p')
if [ "$wifion" = "0" ]; then
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
CRASH_COUNT=0
touch "$EXEC_PATH" && sync
while [ -f "$EXEC_PATH" ]; do
	echo "launch: starting nextui.elf $(date)" >> "$LAUNCH_LOG"
	nextui.elf > "$LOGS_PATH/nextui.txt" 2>&1
	EXIT_CODE=$?
	echo "launch: nextui.elf exited $EXIT_CODE $(date)" >> "$LAUNCH_LOG"
	if [ "$EXIT_CODE" != "0" ]; then
		CRASH_COUNT=$((CRASH_COUNT + 1))
		if [ "$CRASH_COUNT" -ge 5 ]; then
			echo "launch: crash limit reached; powering off" >> "$LAUNCH_LOG"
			rm -f "$EXEC_PATH"
			continue
		fi
	else
		CRASH_COUNT=0
	fi
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
		poweroff_next || poweroff
		exit 0
	fi
	if [ -f "/tmp/reboot" ]; then
		reboot_next || reboot
		exit 0
	fi
done

poweroff_next || poweroff
