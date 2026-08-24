#!/bin/sh

cd "$(dirname "$0")" || exit 1

HOME="$SDCARD_PATH"
CFG="tg5050.cfg"

./NextCommander --config $CFG