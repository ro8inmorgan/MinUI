#!/bin/sh

cd "$(dirname "$0")" || exit 1
./clock.elf # output is intentionally not redirected
