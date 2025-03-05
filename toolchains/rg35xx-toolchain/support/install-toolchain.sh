#!/bin/sh

mkdir -p /opt/rg35xx-toolchain
if [ -d /opt/rg35xx-toolchain/usr ]; then
	rm -fr /opt/rg35xx-toolchain/usr
fi
cp -rf ~/buildroot/output/host/usr/ /opt/rg35xx-toolchain/
# this version of buildroot doesn't have relocate-sdk.sh yet so we bring our own
cp ~/relocate-sdk.sh /opt/rg35xx-toolchain/
cp ~/sdk-location /opt/rg35xx-toolchain/
cp ~/hwcap.h /opt/rg35xx-toolchain/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include/asm/
/opt/rg35xx-toolchain/relocate-sdk.sh