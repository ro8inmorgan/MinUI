#!/bin/sh

cd /opt/
tar --xz -cvf rg35xx-toolchain.tar.xz rg35xx-toolchain/
mv rg35xx-toolchain.tar.xz ~/workspace/

printf "rg35xx-toolchain.tar.xz can be shared as a blob\nby placing in support before calling 'make shell'\n"