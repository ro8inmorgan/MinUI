#!/bin/sh

BUILDROOT_VERSION=2017.11

set -xe

if [ -d ~/buildroot ]; then
	rm -rf ~/buildroot
else
	sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
	locale-gen
fi

cd ~

BUILDROOT_NAME=buildroot-$BUILDROOT_VERSION
wget https://buildroot.org/downloads/$BUILDROOT_NAME.tar.gz
tar -xf ./$BUILDROOT_NAME.tar.gz
rm -f ./$BUILDROOT_NAME.tar.gz
mv ./$BUILDROOT_NAME ./buildroot

# patches for buildroot packages
cd ~/patches
for FILE in $(find . -type f -name "*.patch" 2>/dev/null); do
	cp $FILE ~/buildroot/$FILE
done

cd ~/buildroot
# patches for buildroot itself
patch -p1 < ~/toolchain-expose-BR2_TOOLCHAIN_EXTRA_EXTERNAL_LIBS-for-all-toolchain-types-2017.11.1.diff

cp ~/rg35xx-buildroot-$BUILDROOT_VERSION.config ./.config
if [ -f ~/rg35xx-toolchain.tar.xz ]; then
 	tar -xf ~/rg35xx-toolchain.tar.xz -C /opt
else
	export FORCE_UNSAFE_CONFIGURE=1
	make oldconfig
	make world
	
	~/install-toolchain.sh
fi
