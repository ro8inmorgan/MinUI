#!/bin/sh
# MinUI.pak

# recover from readonly SD card -------------------------------
# touch /mnt/writetest
# sync
# if [ -f /mnt/writetest ] ; then
# 	rm -f /mnt/writetest
# else
# 	e2fsck -p /dev/root > /mnt/SDCARD/RootRecovery.txt
# 	reboot
# fi

export PLATFORM="tg5050"
export SDCARD_PATH="/mnt/SDCARD"
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

#######################################

if [ -f "/tmp/poweroff" ]; then
	poweroff
	exit 0
fi
if [ -f "/tmp/reboot" ]; then
	reboot
	exit 0
fi

#######################################

mkdir -p "$BIOS_PATH"
mkdir -p "$ROMS_PATH"
mkdir -p "$SAVES_PATH"
mkdir -p "$CHEATS_PATH"
mkdir -p "$USERDATA_PATH"
mkdir -p "$LOGS_PATH"
mkdir -p "$HOOKS_PATH"
mkdir -p "$SHARED_USERDATA_PATH/.minui"

TRIMUI_MODEL=$(strings /usr/trimui/bin/MainUI | grep ^Trimui)
export TRIMUI_MODEL
if [ "$TRIMUI_MODEL" = "Trimui Smart Pro S" ]; then
	export DEVICE="smartpros"
fi

export IS_NEXT="yes"

#######################################

# taken from stock launch sequence
sync
echo 3 >/proc/sys/vm/drop_caches
sync

#5V enable
# echo 335 > /sys/class/gpio/export
# printf '%s' out > /sys/class/gpio/gpio335/direction
# printf '%s' 1 > /sys/class/gpio/gpio335/value

#rumble motor PH12
echo 236 >/sys/class/gpio/export
printf '%s' out >/sys/class/gpio/gpio236/direction
printf '%s' 0 >/sys/class/gpio/gpio236/value

#Left/Right Pad PK12/PK16 , run in trimui_inputd
# echo 332 > /sys/class/gpio/export
# printf '%s' out > /sys/class/gpio/gpio332/direction
# printf '%s' 1 > /sys/class/gpio/gpio332/value

# echo 336 > /sys/class/gpio/export
# printf '%s' out > /sys/class/gpio/gpio336/direction
# printf '%s' 1 > /sys/class/gpio/gpio336/value

#DIP Switch PL11 , run in trimui_inputd
# echo 363 > /sys/class/gpio/export
# printf '%s' in > /sys/class/gpio/gpio363/direction

#syslogd -S

#######################################

export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:/usr/trimui/lib:$LD_LIBRARY_PATH
export PATH=$SYSTEM_PATH/bin:/usr/trimui/bin:$PATH

echo "before leds $(cat /proc/uptime)" >>/tmp/nextui_boottime

# leds_off
echo 0 >/sys/class/led_anim/max_scale

# start gpio input daemon
trimui_inputd &

sh "$SYSTEM_PATH/bin/governor.sh" "auto"

echo performance >/sys/devices/platform/soc@3000000/1800000.gpu/devfreq/1800000.gpu/governor

# Very little libretro cores profit from multithreading, even stock OS is
# only very seldomly using more than 1+2 cores. Use as a baseline, the
# higher-end cores can just enable more cores themselves if needed.

# little Cortex-A55 CPU0 - 408Mhz to 1416Mhz
echo 1 >/sys/devices/system/cpu/cpu0/online
echo 1 >/sys/devices/system/cpu/cpu1/online

echo 0 >/sys/devices/system/cpu/cpu3/online
echo 0 >/sys/devices/system/cpu/cpu2/online

# big Cortex-A55 CPU4 - 408Mhz to 2160Mhz
echo 1 >/sys/devices/system/cpu/cpu4/online

echo 0 >/sys/devices/system/cpu/cpu7/online
echo 0 >/sys/devices/system/cpu/cpu6/online
echo 0 >/sys/devices/system/cpu/cpu5/online

keymon.elf & # logging intentionally disabled
batmon.elf & # logging intentionally disabled

# start fresh, will be populated on the next connect
rm -f "$USERDATA_PATH/.asoundrc"
audiomon.elf & # logging intentionally disabled

# wifi handling
wifion=$(nextval.elf wifi | sed -n 's/.*"wifi": \([0-9]*\).*/\1/p')
if [ "$wifion" -eq 1 ]; then
	"$SYSTEM_PATH/etc/wifi/wifi_init.sh" start >/dev/null 2>&1 &
fi
echo "after wifi $(cat /proc/uptime)" >>/tmp/nextui_boottime

# BT handling
bluetoothon=$(nextval.elf bluetooth | sed -n 's/.*"bluetooth": \([0-9]*\).*/\1/p')
if [ "$bluetoothon" -eq 1 ]; then
	"$SYSTEM_PATH/etc/bluetooth/bt_init.sh" start >/dev/null 2>&1 &
fi
echo "after bluetooth $(cat /proc/uptime)" >>/tmp/nextui_boottime

#######################################

AUTO_PATH=$USERDATA_PATH/auto.sh
if [ -f "$AUTO_PATH" ]; then
	echo "before auto.sh $(cat /proc/uptime)" >>/tmp/nextui_boottime
	"$AUTO_PATH"
	echo "after auto.sh $(cat /proc/uptime)" >>/tmp/nextui_boottime
fi

# Composable boot hooks (run after auto.sh for backward compatibility)
"$SYSTEM_PATH/bin/run_hooks.sh" boot.d

cd "$(dirname "$0")" || exit 1

parse_launch_record() {
	[ -f "$NEXT_PATH" ] && [ ! -L "$NEXT_PATH" ] || return 1
	[ "$(wc -c <"$NEXT_PATH")" -le 8192 ] 2>/dev/null || return 1
	exec 3<"$NEXT_PATH" || return 1
	IFS= read -r magic <&3 || {
		exec 3<&-
		return 1
	}
	IFS= read -r count <&3 || {
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
	if [ -n "$LAUNCH_ARGUMENT" ]; then
		HOOK_TYPE="rom"
	else
		HOOK_TYPE="pak"
	fi
	HOOK_CMD="$LAUNCH_PROGRAM${LAUNCH_ARGUMENT:+ $LAUNCH_ARGUMENT}"
	[ -f /tmp/last.txt ] && HOOK_LAST=$(cat /tmp/last.txt) || HOOK_LAST=""
	export HOOK_CMD HOOK_EMU_PATH HOOK_TYPE HOOK_ROM_PATH HOOK_LAST
}

killall -9 show2.elf >/dev/null 2>&1

RUNTIME_PATH="/tmp/nextui-runtime"
ACTIVE_ROM_PATH="$RUNTIME_PATH/active.rom"
export NEXTUI_RUNTIME_PATH="$RUNTIME_PATH"
NEXT_PATH="$RUNTIME_PATH/launch.argv"
EXEC_PATH="$RUNTIME_PATH/nextui.exec"
mkdir -p "$RUNTIME_PATH" && chmod 700 "$RUNTIME_PATH" || exit 1
touch "$EXEC_PATH" && sync
while [ -f "$EXEC_PATH" ]; do
	nextui.elf >"$LOGS_PATH/nextui.txt" 2>&1
	sh "$SYSTEM_PATH/bin/governor.sh" "performance"

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
		sh "$SYSTEM_PATH/bin/governor.sh" "performance"
	else
		printf '%s\n' "NextUI: invalid launch record: $NEXT_PATH" >>"$LOGS_PATH/nextui.txt"
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
