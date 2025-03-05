#! /bin/sh

TOOLCHAIN_VERSION=v0.0.3
TOOLCHAIN_TAR="miyoomini-toolchain.tar.xz"

TOOLCHAIN_ARCH=`uname -m`
if [ "$TOOLCHAIN_ARCH" = "aarch64" ]; then
	TOOLCHAIN_REPO=miyoomini-toolchain-buildroot-aarch64
else
	TOOLCHAIN_REPO=miyoomini-toolchain-buildroot
fi

TOOLCHAIN_URL="https://github.com/shauninman/$TOOLCHAIN_REPO/releases/download/$TOOLCHAIN_VERSION/$TOOLCHAIN_TAR"

if [ -f "./$TOOLCHAIN_TAR" ]; then
	cp "./$TOOLCHAIN_TAR" /opt
	cd /opt
	echo "extracting local toolchain ($TOOLCHAIN_ARCH)"
else
	cd /opt
	wget "$TOOLCHAIN_URL"
	echo "extracting remote toolchain $TOOLCHAIN_VERSION ($TOOLCHAIN_ARCH)"
fi
tar xf "./$TOOLCHAIN_TAR"
rm -rf "./$TOOLCHAIN_TAR"
