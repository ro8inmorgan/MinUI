#!/bin/sh

TF1_PATH=/mnt/mmc
TF2_PATH=/mnt/sdcard
COMPAT_PATH=/mnt/SDCARD
PLATFORM=h700
SYSTEM_PATH="$TF2_PATH/.system/$PLATFORM"
UPDATE_PATH="$TF2_PATH/MinUI.zip"
LOG_PATH=/tmp/nextui-h700.log

trap '' USR1

if mountpoint -q "$TF1_PATH"; then
	LOG_PATH="$TF1_PATH/nextui-h700.log"
fi
: > "$LOG_PATH" 2>/dev/null || true

log() {
	echo "$@" >> "$LOG_PATH" 2>/dev/null || true
}

has_pakz() {
	for pakz in "$TF2_PATH"/*.pakz; do
		[ -e "$pakz" ] && return 0
	done
	return 1
}

mount_tf2() {
	mkdir -p "$TF2_PATH"
	if mountpoint -q "$TF2_PATH"; then
		return 0
	fi
	mount -t vfat -o rw,utf8,noatime /dev/mmcblk1p1 "$TF2_PATH" 2>/dev/null ||
		mount -t exfat -o rw,noatime /dev/mmcblk1p1 "$TF2_PATH" 2>/dev/null ||
		mount -o rw,noatime /dev/mmcblk1p1 "$TF2_PATH" 2>/dev/null
}

repair_tf2() {
	FSTYPE=$(blkid -o value -s TYPE /dev/mmcblk1p1 2>/dev/null)
	case "$FSTYPE" in
		vfat|msdos|fat)
			command -v fsck.fat >/dev/null 2>&1 && fsck.fat -a /dev/mmcblk1p1 >> "$LOG_PATH" 2>&1 || true
			;;
		exfat)
			command -v fsck.exfat >/dev/null 2>&1 && fsck.exfat -a /dev/mmcblk1p1 >> "$LOG_PATH" 2>&1 || true
			;;
	esac
}

ensure_compat_path() {
	if mountpoint -q "$COMPAT_PATH"; then
		return 0
	fi
	if [ -L "$COMPAT_PATH" ]; then
		return 0
	fi
	if [ -e "$COMPAT_PATH" ]; then
		mount --bind "$TF2_PATH" "$COMPAT_PATH" 2>/dev/null || true
	else
		ln -s "$TF2_PATH" "$COMPAT_PATH" 2>/dev/null || true
	fi
}

extract_payload() {
	if [ -x /tmp/nextui-h700-unzip ]; then
		return 0
	fi

	LINE=$(($(grep -na '^BINARY' "$0" | cut -d ':' -f 1 | head -1) + 1))
	tail -n +"$LINE" "$0" > /tmp/nextui-h700-data.tar.gz
	mkdir -p /tmp/nextui-h700
	if ! tar -xzf /tmp/nextui-h700-data.tar.gz -C /tmp/nextui-h700 >> "$LOG_PATH" 2>&1; then
		log "embedded unzip extraction failed"
		return 1
	fi
	cp /tmp/nextui-h700/unzip /tmp/nextui-h700-unzip
	chmod +x /tmp/nextui-h700-unzip
}

find_unzip() {
	for unzip_path in /usr/bin/unzip /bin/unzip /mnt/vendor/bin/unzip /tmp/nextui-h700-unzip; do
		if [ -x "$unzip_path" ]; then
			echo "$unzip_path"
			return 0
		fi
	done

	if extract_payload; then
		echo /tmp/nextui-h700-unzip
		return 0
	fi

	return 1
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
	repair_tf2
fi

if ! mount_tf2; then
	log "TF2 mount failed"
	fallback_stock
fi

ensure_compat_path

if [ -f "$UPDATE_PATH" ]; then
	log "install/update zip detected"
	if [ ! -x "$TF2_PATH/.tmp_update/$PLATFORM.sh" ]; then
		UNZIP_CMD=$(find_unzip)
		if [ -z "$UNZIP_CMD" ]; then
			log "unzip helper unavailable"
			fallback_stock
		fi
		if ! "$UNZIP_CMD" -o "$UPDATE_PATH" ".tmp_update/*" -d "$TF2_PATH" >> "$LOG_PATH" 2>&1; then
			log "updater bootstrap extraction failed"
			fallback_stock
		fi
	fi
fi

if [ -x "$TF2_PATH/.tmp_update/$PLATFORM.sh" ] && { [ -f "$UPDATE_PATH" ] || has_pakz; }; then
	"$TF2_PATH/.tmp_update/$PLATFORM.sh" >> "$LOG_PATH" 2>&1
elif [ -x "$SYSTEM_PATH/bin/install.sh" ] && [ -f "$UPDATE_PATH" ]; then
	"$SYSTEM_PATH/bin/install.sh" >> "$LOG_PATH" 2>&1
fi

if [ -x "$SYSTEM_PATH/paks/MinUI.pak/launch.sh" ]; then
	exec "$SYSTEM_PATH/paks/MinUI.pak/launch.sh"
fi

log "NextUI launch script missing"
fallback_stock
