#!/bin/sh

# becomes /usr/trimui/bin/runtrimui.sh on tg5040/tg3040/tg5050/tg4040

# wait for SDCARD mounted
echo "before mount $(cat /proc/uptime)" >>/tmp/nextui_boottime
mounted=$(cat /proc/mounts | grep -i SDCARD)
cnt=0
while [ "$mounted" = "" ] && [ "$cnt" -lt 6 ]; do
   sleep 0.5
   cnt=$(expr "$cnt" + 1)
   mounted=$(cat /proc/mounts | grep -i SDCARD)
done
echo "after mount $(cat /proc/uptime)" >>/tmp/nextui_boottime

SDCARD_PATH=${SDCARD_PATH:-/mnt/SDCARD}
UPDATER_PATH="$SDCARD_PATH/.tmp_update/updater"
UPDATE_STATE="$SDCARD_PATH/.nextui-update-state"
UPDATE_BACKUP="$SDCARD_PATH/.nextui-update-backup"

if [ -f "$UPDATE_STATE" ]; then
   UPDATE_PLATFORM=$(cat "$UPDATE_STATE")
   case "$UPDATE_PLATFORM" in tg5040 | tg5050) ;; *) UPDATE_PLATFORM="" ;; esac
   [ ! -d "$UPDATE_BACKUP/system" ] || {
      rm -rf "$SDCARD_PATH/.system"
      mv "$UPDATE_BACKUP/system" "$SDCARD_PATH/.system"
   }
   [ ! -d "$UPDATE_BACKUP/tmp_update" ] || {
      rm -rf "$SDCARD_PATH/.tmp_update"
      mv "$UPDATE_BACKUP/tmp_update" "$SDCARD_PATH/.tmp_update"
   }
   [ -z "$UPDATE_PLATFORM" ] || [ ! -d "$UPDATE_BACKUP/tools" ] || {
      rm -rf "$SDCARD_PATH/Tools/$UPDATE_PLATFORM"
      mkdir -p "$SDCARD_PATH/Tools"
      mv "$UPDATE_BACKUP/tools" "$SDCARD_PATH/Tools/$UPDATE_PLATFORM"
   }
   rm -rf "$UPDATE_BACKUP"
   rm -f "$UPDATE_STATE"
   sync
elif [ -d "$UPDATE_BACKUP" ]; then
   [ ! -d "$UPDATE_BACKUP/system" ] || {
      rm -rf "$SDCARD_PATH/.system"
      mv "$UPDATE_BACKUP/system" "$SDCARD_PATH/.system"
   }
   [ ! -d "$UPDATE_BACKUP/tmp_update" ] || {
      rm -rf "$SDCARD_PATH/.tmp_update"
      mv "$UPDATE_BACKUP/tmp_update" "$SDCARD_PATH/.tmp_update"
   }
   rm -rf "$UPDATE_BACKUP"
   sync
fi

if [ -x "$UPDATER_PATH" ]; then
   "$UPDATER_PATH"
   status=$?
   [ "${NEXTUI_TEST:-0}" != 1 ] || exit "$status"
else
   /usr/trimui/bin/runtrimui-original.sh
fi
