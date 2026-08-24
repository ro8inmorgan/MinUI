#!/bin/sh
# NOTE: becomes .tmp_update/tg5040.sh

PLATFORM="tg5040"
SDCARD_PATH="${SDCARD_PATH:-/mnt/SDCARD}"
UPDATE_PATH="$SDCARD_PATH/MinUI.zip"
SYSTEM_PATH="$SDCARD_PATH/.system"
BOOT_DIR=$(dirname "$0")
UPDATE_LOG="$SDCARD_PATH/NextUI-update.log"
UPDATE_MARKER="$SDCARD_PATH/.nextui-update-failed"
UPDATE_STATE="$SDCARD_PATH/.nextui-update-state"

update_fail() {
	printf '%s\n' "$*" >>"$UPDATE_LOG"
	printf '%s\n' "$*" >"$UPDATE_MARKER"
	rm -f "$UPDATE_STATE"
	return 1
}

install_update() {
	UNZIP="${NEXTUI_UNZIP:-$BOOT_DIR/$PLATFORM/unzip}"
	STAGE="$SDCARD_PATH/.nextui-stage.$$"
	BACKUP="$SDCARD_PATH/.nextui-update-backup"
	SYSTEM_NEW=0
	TMP_NEW=0
	TOOLS_NEW=0
	TOOLS_PATH="$SDCARD_PATH/Tools/$PLATFORM"

	rollback() {
		[ "$TOOLS_NEW" = 1 ] && rm -rf "$TOOLS_PATH"
		[ "$TMP_NEW" = 1 ] && rm -rf "$SDCARD_PATH/.tmp_update"
		[ "$SYSTEM_NEW" = 1 ] && rm -rf "$SYSTEM_PATH"
		[ -d "$BACKUP/system" ] && mv "$BACKUP/system" "$SYSTEM_PATH"
		[ -d "$BACKUP/tmp_update" ] && mv "$BACKUP/tmp_update" "$SDCARD_PATH/.tmp_update"
		[ -d "$BACKUP/tools" ] && mv "$BACKUP/tools" "$TOOLS_PATH"
		rm -f "$UPDATE_STATE"
	}

	checkpoint() {
		sync
		[ "${NEXTUI_TEST_KILL_AFTER:-}" != "$1" ] || kill -9 "$$"
	}

	[ -x "$UNZIP" ] || update_fail "update failed: unzip is unavailable" || return 1
	mkdir "$STAGE" || update_fail "update failed: cannot create staging" || return 1
	[ ! -e "$BACKUP" ] || {
		rm -rf "$STAGE"
		update_fail "update failed: unfinished update recovery required"
		return 1
	}
	printf '%s\n' "$PLATFORM" >"$UPDATE_STATE" || {
		rm -rf "$STAGE"
		update_fail "update failed: cannot record transaction"
		return 1
	}
	sync
	checkpoint transaction-state
	mkdir "$BACKUP" || {
		rm -rf "$STAGE"
		update_fail "update failed: cannot create backup"
		return 1
	}
	checkpoint backup-created
	checkpoint archive-test
	"$UNZIP" -t "$UPDATE_PATH" >>"$UPDATE_LOG" 2>&1 || {
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: archive test failed"
		return 1
	}
	"$UNZIP" -Z1 "$UPDATE_PATH" | awk '/^\/|(^|\/)\.\.($|\/)/ { bad = 1 } END { exit bad }' || {
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: unsafe archive path"
		return 1
	}
	checkpoint extraction
	"$UNZIP" -o "$UPDATE_PATH" -d "$STAGE" >>"$UPDATE_LOG" 2>&1 || {
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: extraction failed"
		return 1
	}
	checkpoint checksum
	[ -f "$STAGE/SHA256SUMS" ] && (cd "$STAGE" && sha256sum -c SHA256SUMS) >>"$UPDATE_LOG" 2>&1 || {
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: SHA-256 manifest check failed"
		return 1
	}
	[ -d "$STAGE/.system/$PLATFORM/bin" ] && [ -x "$STAGE/.system/$PLATFORM/paks/MinUI.pak/launch.sh" ] && [ -x "$STAGE/.tmp_update/updater" ] && [ -x "$STAGE/.tmp_update/$PLATFORM.sh" ] || {
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: required payload missing"
		return 1
	}
	checkpoint prepared

	[ ! -e "$SYSTEM_PATH" ] || { mv "$SYSTEM_PATH" "$BACKUP/system" || {
		rollback
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: cannot back up runtime"
		return 1
	}; }
	checkpoint system-backup
	mv "$STAGE/.system" "$SYSTEM_PATH" && SYSTEM_NEW=1 || {
		rollback
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: runtime commit failed"
		return 1
	}
	checkpoint system-installed
	[ ! -e "$SDCARD_PATH/.tmp_update" ] || { mv "$SDCARD_PATH/.tmp_update" "$BACKUP/tmp_update" || {
		rollback
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: cannot back up updater"
		return 1
	}; }
	checkpoint updater-backup
	mv "$STAGE/.tmp_update" "$SDCARD_PATH/.tmp_update" && TMP_NEW=1 || {
		rollback
		rm -rf "$STAGE" "$BACKUP"
		update_fail "update failed: updater commit failed"
		return 1
	}
	checkpoint updater-installed
	if [ -d "$STAGE/Tools/$PLATFORM" ]; then
		mkdir -p "$SDCARD_PATH/Tools" || {
			rollback
			rm -rf "$STAGE" "$BACKUP"
			update_fail "update failed: cannot create tools directory"
			return 1
		}
		[ ! -e "$TOOLS_PATH" ] || { mv "$TOOLS_PATH" "$BACKUP/tools" || {
			rollback
			rm -rf "$STAGE" "$BACKUP"
			update_fail "update failed: cannot back up tools"
			return 1
		}; }
		checkpoint tools-backup
		mv "$STAGE/Tools/$PLATFORM" "$TOOLS_PATH" && TOOLS_NEW=1 || {
			rollback
			rm -rf "$STAGE" "$BACKUP"
			update_fail "update failed: tools commit failed"
			return 1
		}
		checkpoint tools-installed
	fi
	if [ -x "$SYSTEM_PATH/$PLATFORM/bin/install.sh" ]; then
		"$SYSTEM_PATH/$PLATFORM/bin/install.sh" >>"$UPDATE_LOG" 2>&1 || {
			rollback
			rm -rf "$STAGE" "$BACKUP"
			update_fail "update failed: post-install failed"
			return 1
		}
	fi
	rm -rf "$STAGE" "$BACKUP"
	rm -f "$UPDATE_STATE" "$UPDATE_PATH" "$UPDATE_MARKER"
	sync
	return 0
}

if [ "${NEXTUI_TEST:-0}" = 1 ]; then
	[ ! -f "$UPDATE_PATH" ] || install_update
	exit $?
fi

export LD_LIBRARY_PATH=/usr/trimui/lib:$LD_LIBRARY_PATH
export PATH=/usr/trimui/bin:$PATH

TRIMUI_MODEL=$(strings /usr/trimui/bin/MainUI | grep ^Trimui)
if [ "$TRIMUI_MODEL" = "Trimui Brick" ]; then
	DEVICE="brick"
elif [ "$TRIMUI_MODEL" = "Trimui Brick Pro" ]; then
	DEVICE="brickpro"
fi

# only show splash if either UPDATE_PATH or pakz files exist
SHOW_SPLASH="no"
if [ -f "$UPDATE_PATH" ]; then
	SHOW_SPLASH="yes"
else
	for pakz in "$SDCARD_PATH"/*.pakz; do
		if [ -e "$pakz" ]; then
			SHOW_SPLASH="yes"
			break
		fi
	done
fi
LOGO_PATH="logo.png"
# If the user put a custom logo under /mnt/SDCARD/.media/splash_logo.png, use that instead
if [ -f "$SDCARD_PATH/.media/splash_logo.png" ]; then
	LOGO_PATH="$SDCARD_PATH/.media/splash_logo.png"
fi

if [ "$SHOW_SPLASH" = "yes" ]; then
	cd "$BOOT_DIR/$PLATFORM" || exit 1
	if [ "$DEVICE" = "brick" ] || [ "$DEVICE" = "brickpro" ]; then
		./show2.elf --mode=daemon --image="$LOGO_PATH" --text="Installing..." --logoheight=144 --fontsize=32 --progress=-1 &
	else
		./show2.elf --mode=daemon --image="$LOGO_PATH" --text="Installing..." --logoheight=128 --progress=-1 &
	fi
	#sleep 0.5
	#SHOW_PID=$!
fi

echo userspace >/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
CPU_PATH=/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
CPU_SPEED_PERF=2000000
echo $CPU_SPEED_PERF >$CPU_PATH

##Remove Old Led Daemon
if [ -f "/etc/LedControl" ]; then
	rm -Rf "/etc/LedControl"
fi
if [ -f "/etc/init.d/lcservice" ]; then
	/etc/init.d/lcservice disable
	rm /etc/init.d/lcservice
fi

# leds_off
echo 0 >/sys/class/led_anim/max_scale

# generic NextUI package install
for pakz in "$SDCARD_PATH"/*.pakz; do
	if [ ! -e "$pakz" ]; then continue; fi
	echo "TEXT:Extracting $pakz" >/tmp/show2.fifo
	cd "$BOOT_DIR/$PLATFORM" || exit 1

	./unzip -o -d "$SDCARD_PATH" "$pakz" # >> $pakz.txt
	rm -f "$pakz"

	# run postinstall if present
	if [ -f "$SDCARD_PATH/post_install.sh" ]; then
		echo "TEXT:Installing $pakz" >/tmp/show2.fifo
		"$SDCARD_PATH/post_install.sh" # > $pakz_post.txt
		rm -f $SDCARD_PATH/post_install.sh
	fi
done

# install/update
if [ -f "$UPDATE_PATH" ]; then
	if [ -d "$SYSTEM_PATH" ]; then
		echo "TEXT:Updating NextUI" >/tmp/show2.fifo
	else
		echo "TEXT:Installing NextUI" >/tmp/show2.fifo
	fi
	install_update || exit 1
fi

#kill $SHOW_PID

LAUNCH_PATH="$SYSTEM_PATH/$PLATFORM/paks/MinUI.pak/launch.sh"
if [ -f "$LAUNCH_PATH" ]; then
	"$LAUNCH_PATH"
fi
killall trimui_inputd

poweroff # under no circumstances should stock be allowed to touch this card
while true; do
	echo "Waiting for poweroff."
	sleep 1
done
