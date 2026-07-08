#!/bin/sh

TF1_PATH=/mnt/mmc
TF2_PATH=/mnt/sdcard
COMPAT_PATH=/mnt/SDCARD
PLATFORM=h700
SYSTEM_PATH="$TF2_PATH/.system/$PLATFORM"
UPDATE_PATH="$TF2_PATH/MinUI.zip"
LOG_PATH="$TF1_PATH/nextui-h700.log"

log() {
	echo "$@" >> "$LOG_PATH" 2>/dev/null || true
}

mount_tf2() {
	mkdir -p "$TF2_PATH"
	if mountpoint -q "$TF2_PATH"; then
		return 0
	fi
	mount -t vfat -o rw,utf8,noatime /dev/mmcblk1p1 "$TF2_PATH" 2>/dev/null
}

ensure_compat_path() {
	if [ ! -e "$COMPAT_PATH" ]; then
		ln -s "$TF2_PATH" "$COMPAT_PATH" 2>/dev/null || true
	fi
}

extract_payload() {
	if [ -x /tmp/nextui-h700-unzip ]; then
		return 0
	fi

	LINE=$(($(grep -na '^BINARY' "$0" | cut -d ':' -f 1 | tail -1) + 1))
	tail -n +"$LINE" "$0" > /tmp/nextui-h700-data.tar.gz
	mkdir -p /tmp/nextui-h700
	tar -xzf /tmp/nextui-h700-data.tar.gz -C /tmp/nextui-h700 >/dev/null 2>&1 || return 1
	cp /tmp/nextui-h700/unzip /tmp/nextui-h700-unzip
	chmod +x /tmp/nextui-h700-unzip
}

fallback_stock() {
	log "falling back to stock frontend"
	if [ -x /mnt/vendor/bin/dmenu.bin ]; then
		exec /mnt/vendor/bin/dmenu.bin
	fi
	exit 1
}

if [ -f /mnt/vendor/muos1.ini ] || [ -f /mnt/vendor/muos2.ini ]; then
	log "stockmod muOS override is present; NextUI requires the stock boot target"
fi

if ! mount_tf2; then
	log "TF2 mount failed"
	fallback_stock
fi

ensure_compat_path

if [ -f "$UPDATE_PATH" ]; then
	log "install/update zip detected"
	extract_payload || fallback_stock
	/tmp/nextui-h700-unzip -o "$UPDATE_PATH" -d "$TF2_PATH" >> "$LOG_PATH" 2>&1
	rm -f "$UPDATE_PATH"

	if [ -x "$TF2_PATH/.tmp_update/$PLATFORM.sh" ]; then
		"$TF2_PATH/.tmp_update/$PLATFORM.sh" >> "$LOG_PATH" 2>&1
	elif [ -x "$SYSTEM_PATH/bin/install.sh" ]; then
		"$SYSTEM_PATH/bin/install.sh" >> "$LOG_PATH" 2>&1
	fi
fi

if [ -x "$SYSTEM_PATH/paks/MinUI.pak/launch.sh" ]; then
	exec "$SYSTEM_PATH/paks/MinUI.pak/launch.sh"
fi

log "NextUI launch script missing"
fallback_stock
