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
DEBUG_KEEP_NETWORK_PATH="$USERDATA_PATH/debug-keep-network"
DEBUG_WIFI_CONF="$USERDATA_PATH/debug-wifi.conf"

if ! mountpoint -q "$SDCARD_PATH" && [ ! -L "$SDCARD_PATH" ]; then
	if [ -e "$SDCARD_PATH" ]; then
		mount --bind "$REAL_SDCARD_PATH" "$SDCARD_PATH" 2>/dev/null || true
	else
		ln -s "$REAL_SDCARD_PATH" "$SDCARD_PATH" 2>/dev/null || true
	fi
fi

export LD_LIBRARY_PATH="$SYSTEM_PATH/$PLATFORM/lib:/usr/lib:/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH"
export PATH="$SYSTEM_PATH/$PLATFORM/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"

cd "$(dirname "$0")/$PLATFORM" || exit 1

mkdir -p "$LOGS_PATH"
echo "install: starting $(date)" > "$INSTALL_LOG"

log_install() {
	echo "install: $* $(date)" >> "$INSTALL_LOG"
}

read_debug_wifi_value() {
	key="$1"
	[ -f "$DEBUG_WIFI_CONF" ] || return 0
	sed -n "s/^$key=//p" "$DEBUG_WIFI_CONF" | tail -n 1 | sed 's/^"//;s/"$//' | tr -d '\r'
}

connect_debug_wifi() {
	[ -f "$DEBUG_WIFI_CONF" ] || return 0
	DEBUG_WIFI_SSID="$(read_debug_wifi_value SSID)"
	DEBUG_WIFI_PASSWORD="$(read_debug_wifi_value PASSWORD)"
	DEBUG_WIFI_IFACE="$(read_debug_wifi_value INTERFACE)"
	[ -n "$DEBUG_WIFI_IFACE" ] || DEBUG_WIFI_IFACE="wlan0"
	if [ -z "$DEBUG_WIFI_SSID" ]; then
		log_install "debug-wifi.conf present without SSID="
		return 0
	fi

	log_install "connecting debug wifi SSID=$DEBUG_WIFI_SSID IFACE=$DEBUG_WIFI_IFACE"
	for attempt in 1 2 3; do
		nmcli dev wifi rescan ifname "$DEBUG_WIFI_IFACE" >> "$INSTALL_LOG" 2>&1 || true
		if [ -n "$DEBUG_WIFI_PASSWORD" ]; then
			nmcli --wait 20 dev wifi connect "$DEBUG_WIFI_SSID" password "$DEBUG_WIFI_PASSWORD" ifname "$DEBUG_WIFI_IFACE" >> "$INSTALL_LOG" 2>&1 && return 0
		else
			nmcli --wait 20 dev wifi connect "$DEBUG_WIFI_SSID" ifname "$DEBUG_WIFI_IFACE" >> "$INSTALL_LOG" 2>&1 && return 0
		fi
		log_install "debug wifi attempt $attempt failed"
		sleep 2
	done
}

start_debug_network() {
	log_install "starting stock network services for debug"
	rfkill.elf unblock wifi >> "$INSTALL_LOG" 2>&1 || rfkill unblock wifi >> "$INSTALL_LOG" 2>&1 || true
	systemctl start NetworkManager >> "$INSTALL_LOG" 2>&1 || true
	nmcli networking on >> "$INSTALL_LOG" 2>&1 || true
	nmcli radio wifi on >> "$INSTALL_LOG" 2>&1 || true
	connect_debug_wifi
	systemctl start ssh >> "$INSTALL_LOG" 2>&1 ||
		systemctl start sshd >> "$INSTALL_LOG" 2>&1 ||
		/usr/sbin/sshd >> "$INSTALL_LOG" 2>&1 ||
		true
	ip addr show >> "$INSTALL_LOG" 2>&1 || true
}

show_progress() {
	log_install "$1"
	if [ -x ./show2.elf ]; then
		./show2.elf --mode=daemon --image="logo.png" --text="$1" --logoheight=128 --progress=-1 >/dev/null 2>&1 &
	fi
}

show_progress "Installing..."
sh "$SYSTEM_PATH/$PLATFORM/bin/governor.sh" performance 2>/dev/null || true
if [ -f "$DEBUG_KEEP_NETWORK_PATH" ] || [ -f "$DEBUG_WIFI_CONF" ]; then
	start_debug_network
else
	systemctl stop NetworkManager 2>/dev/null || true
fi
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
