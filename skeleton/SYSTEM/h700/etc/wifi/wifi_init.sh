#!/bin/sh

WIFI_INTERFACE="${WIFI_INTERFACE:-wlan0}"
WIFI_DIR="/tmp/wifi"
WIFI_SOCK_DIR="$WIFI_DIR/sockets"
WPA_SUPPLICANT_CONF="$WIFI_DIR/wpa_supplicant.conf"

write_default_config() {
	mkdir -p "$WIFI_SOCK_DIR"
	if [ ! -f "$WPA_SUPPLICANT_CONF" ]; then
		cat > "$WPA_SUPPLICANT_CONF" << EOF
ctrl_interface=$WIFI_SOCK_DIR
disable_scan_offload=1
update_config=1
wowlan_triggers=any

EOF
	fi
}

start_dhcp() {
	if command -v dhclient >/dev/null 2>&1; then
		dhclient -nw "$WIFI_INTERFACE" >/dev/null 2>&1 || true
	elif command -v udhcpc >/dev/null 2>&1; then
		udhcpc -i "$WIFI_INTERFACE" -b >/dev/null 2>&1 || true
	fi
}

stop_dhcp() {
	if command -v dhclient >/dev/null 2>&1; then
		dhclient -r "$WIFI_INTERFACE" >/dev/null 2>&1 || true
	fi
	killall udhcpc 2>/dev/null || true
}

start() {
	systemctl stop NetworkManager 2>/dev/null || true
	rfkill.elf unblock wifi 2>/dev/null || rfkill unblock wifi 2>/dev/null || true
	ip link set "$WIFI_INTERFACE" up 2>/dev/null || true
	write_default_config

	killall wpa_supplicant 2>/dev/null || true
	wpa_supplicant -B -i "$WIFI_INTERFACE" -c "$WPA_SUPPLICANT_CONF" -C "$WIFI_SOCK_DIR" >/dev/null 2>&1
	start_dhcp
}

stop() {
	wpa_cli -p "$WIFI_SOCK_DIR" -i "$WIFI_INTERFACE" terminate >/dev/null 2>&1 || true
	killall wpa_supplicant 2>/dev/null || true
	stop_dhcp
	ip link set "$WIFI_INTERFACE" down 2>/dev/null || true
	rfkill.elf block wifi 2>/dev/null || rfkill block wifi 2>/dev/null || true
}

case "$1" in
	start|"")
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		sleep 1
		start
		;;
	*)
		echo "Usage: $0 {start|stop|restart}"
		exit 1
		;;
esac
