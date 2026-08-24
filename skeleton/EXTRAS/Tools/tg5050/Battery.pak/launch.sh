#!/bin/sh

cd "$(dirname "$0")" || exit 1
./battery.elf # output is intentionally not redirected
