#!/bin/sh

cd "$(dirname "$0")" || exit 1

HOME="$SDCARD_PATH"
# brick and brickpro
if [ "$DEVICE" = "brick" ] || [ "$DEVICE" = "brickpro" ]; then
    CFG="tg3040.cfg"
else
    CFG="tg5040.cfg"
fi

./NextCommander --config $CFG