#!/bin/sh
# NOTE: becomes .tmp_update/h700.sh

PLATFORM="h700"
SDCARD_PATH="/mnt/SDCARD"
REAL_SDCARD_PATH="/mnt/sdcard"
UPDATE_PATH="$SDCARD_PATH/MinUI.zip"
PAKZ_PATH="$SDCARD_PATH/*.pakz"
SYSTEM_PATH="$SDCARD_PATH/.system"
USERDATA_PATH="$SDCARD_PATH/.userdata/h700"
LOGS_PATH="$USERDATA_PATH/logs"
INSTALL_LOG="$LOGS_PATH/install.txt"
SCRIPT_DIR="$(dirname "$0")"
COMMON_PATH="$SCRIPT_DIR/$PLATFORM/shim-common.sh"

if [ ! -f "$COMMON_PATH" ]; then
	echo "install: missing $COMMON_PATH $(date)" >> /tmp/nextui-h700-install.log 2>/dev/null || true
	exit 1
fi
SHIM_LOG_PATH="$INSTALL_LOG"
SHIM_LOG_PREFIX="install: "
SHIM_LOG_DATES=1
. "$COMMON_PATH"

ensure_compat_path "$REAL_SDCARD_PATH" "$SDCARD_PATH"

export LD_LIBRARY_PATH="$SYSTEM_PATH/$PLATFORM/lib:/usr/lib:/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH"
export PATH="$SYSTEM_PATH/$PLATFORM/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export PLATFORM

cd "$SCRIPT_DIR/$PLATFORM" || exit 1

mkdir -p "$LOGS_PATH"
echo "install: starting $(date)" > "$INSTALL_LOG"

log_install() {
	shim_log "$@"
}

show_progress() {
	log_install "$1"
	if [ -x ./show2.elf ]; then
		./show2.elf --mode=daemon --image="logo.png" --text="$1" --logoheight=128 --progress=-1 >/dev/null 2>&1 &
	fi
}

show_progress "Installing..."
sh "$SYSTEM_PATH/$PLATFORM/bin/governor.sh" performance 2>/dev/null || true
systemctl stop NetworkManager 2>/dev/null || true
killall brightCtrl.bin cexpert 2>/dev/null || true

for pakz in $PAKZ_PATH; do
	if [ ! -e "$pakz" ]; then continue; fi
	echo "TEXT:Extracting $pakz" > /tmp/show2.fifo 2>/dev/null || true
	log_install "extracting $pakz"
	./unzip -o -d "$SDCARD_PATH" "$pakz" >> "$INSTALL_LOG" 2>&1
	unzip_status=$?
	log_install "extracting $pakz exited $unzip_status"
	[ "$unzip_status" = "0" ] || exit "$unzip_status"
	rm -f "$pakz"

	if [ -f "$SDCARD_PATH/post_install.sh" ]; then
		echo "TEXT:Installing $pakz" > /tmp/show2.fifo 2>/dev/null || true
		log_install "running post_install.sh for $pakz"
		sh "$SDCARD_PATH/post_install.sh" >> "$INSTALL_LOG" 2>&1
		post_status=$?
		log_install "post_install.sh exited $post_status"
		[ "$post_status" = "0" ] || exit "$post_status"
		rm -f "$SDCARD_PATH/post_install.sh"
	fi
done

if [ -f "$UPDATE_PATH" ]; then
	if [ -d "$SYSTEM_PATH/$PLATFORM" ]; then
		echo "TEXT:Updating NextUI" > /tmp/show2.fifo 2>/dev/null || true
	else
		echo "TEXT:Installing NextUI" > /tmp/show2.fifo 2>/dev/null || true
	fi

	rm -rf "$SYSTEM_PATH/$PLATFORM/bin"
	rm -rf "$SYSTEM_PATH/$PLATFORM/lib"
	rm -rf "$SYSTEM_PATH/$PLATFORM/paks/MinUI.pak"

	log_install "extracting $UPDATE_PATH"
	./unzip -o "$UPDATE_PATH" -d "$SDCARD_PATH" >> "$INSTALL_LOG" 2>&1
	unzip_status=$?
	log_install "extracting $UPDATE_PATH exited $unzip_status"
	[ "$unzip_status" = "0" ] || exit "$unzip_status"
	rm -f "$UPDATE_PATH"
fi

if [ -x "$SYSTEM_PATH/$PLATFORM/bin/install.sh" ]; then
	log_install "running platform install.sh"
	"$SYSTEM_PATH/$PLATFORM/bin/install.sh" >> "$INSTALL_LOG" 2>&1
	install_status=$?
	log_install "platform install.sh exited $install_status"
	[ "$install_status" = "0" ] || exit "$install_status"
fi

LAUNCH_PATH="$SYSTEM_PATH/$PLATFORM/paks/MinUI.pak/launch.sh"
if [ -x "$LAUNCH_PATH" ]; then
	log_install "exec $LAUNCH_PATH"
	exec "$LAUNCH_PATH"
fi

log_install "missing launch path; powering off"
poweroff
