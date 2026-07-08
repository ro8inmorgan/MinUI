#!/bin/sh

SOURCE=boot.sh
TARGET=dmenu.bin

mkdir -p output
cp ../other/unzip60/unzip ./output/

cd output || exit 1
tar -czf data unzip
cat ../$SOURCE > $TARGET
echo BINARY >> $TARGET
cat data >> $TARGET
echo >> $TARGET
chmod +x $TARGET
