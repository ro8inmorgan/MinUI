#!/bin/sh

SOURCE=boot.sh
TARGET=dmenu.bin

mkdir -p output
${CROSS_COMPILE}gcc -Os -s fbsplash.c -o ./output/fbsplash
cp ../other/unzip60/unzip ./output/

cd output || exit 1
tar -czf data fbsplash unzip
cat ../$SOURCE > $TARGET
echo BINARY >> $TARGET
cat data >> $TARGET
echo >> $TARGET
chmod +x $TARGET
