#!/bin/sh

DEVICE_NAME="${RGXX_MODEL:-Anbernic RG XX} (NextUI)"

start_bt() {
	rfkill.elf unblock bluetooth 2>/dev/null || rfkill unblock bluetooth 2>/dev/null || true
	systemctl start bluetooth 2>/dev/null || true
	pidof bluetoothd >/dev/null 2>&1 || bluetoothd >/dev/null 2>&1 &

	hciconfig hci0 up 2>/dev/null || true

	if command -v bluealsa >/dev/null 2>&1 && ! pidof bluealsa >/dev/null 2>&1; then
		bluealsa -p a2dp-source >/dev/null 2>&1 &
		sleep 1
	fi

	bluetoothctl power on >/dev/null 2>&1 || true
	bluetoothctl discoverable on >/dev/null 2>&1 || true
	bluetoothctl pairable on >/dev/null 2>&1 || true
	bluetoothctl agent NoInputNoOutput >/dev/null 2>&1 || true
	bluetoothctl default-agent >/dev/null 2>&1 || true
	bluetoothctl system-alias "$DEVICE_NAME" >/dev/null 2>&1 || true
}

stop_bt() {
	bluetoothctl power off >/dev/null 2>&1 || true
	killall bluetoothctl 2>/dev/null || true
	killall bluealsa 2>/dev/null || true
	hciconfig hci0 down 2>/dev/null || true
	systemctl stop bluetooth 2>/dev/null || killall bluetoothd 2>/dev/null || true
	rfkill.elf block bluetooth 2>/dev/null || rfkill block bluetooth 2>/dev/null || true
}

case "$1" in
	start|"")
		start_bt
		;;
	stop)
		stop_bt
		;;
	restart)
		stop_bt
		sleep 1
		start_bt
		;;
	*)
		echo "Usage: $0 {start|stop|restart}"
		exit 1
		;;
esac
