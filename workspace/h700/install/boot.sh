#!/bin/sh
# NOTE: becomes .tmp_update/h700.sh

PLATFORM="h700"
SDCARD_PATH="/mnt/SDCARD"
REAL_SDCARD_PATH="/mnt/sdcard"
UPDATE_PATH="$SDCARD_PATH/MinUI.zip"
PAKZ_PATH="$SDCARD_PATH/*.pakz"
SYSTEM_PATH="$SDCARD_PATH/.system"

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

show_progress() {
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
	./unzip -o -d "$SDCARD_PATH" "$pakz"
	rm -f "$pakz"

	if [ -f "$SDCARD_PATH/post_install.sh" ]; then
		echo "TEXT:Installing $pakz" > /tmp/show2.fifo 2>/dev/null || true
		sh "$SDCARD_PATH/post_install.sh"
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

	./unzip -o "$UPDATE_PATH" -d "$SDCARD_PATH"
	rm -f "$UPDATE_PATH"
fi

if [ -x "$SYSTEM_PATH/$PLATFORM/bin/install.sh" ]; then
	"$SYSTEM_PATH/$PLATFORM/bin/install.sh"
fi

LAUNCH_PATH="$SYSTEM_PATH/$PLATFORM/paks/MinUI.pak/launch.sh"
if [ -x "$LAUNCH_PATH" ]; then
	exec "$LAUNCH_PATH"
fi

poweroff
