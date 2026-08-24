#!/bin/sh

cd "$(dirname "$0")" || exit 1
./gametime.elf # output is intentionally not redirected
