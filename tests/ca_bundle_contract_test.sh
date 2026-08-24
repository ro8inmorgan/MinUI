#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
mkdir -p "${BUILD_DIR:-/tmp/nextui-tests}"
TEST_ROOT=$(mktemp -d "${BUILD_DIR:-/tmp/nextui-tests}/ca.XXXXXX")
mkdir "$TEST_ROOT/SDL"
: >"$TEST_ROOT/SDL/SDL.h"
: >"$TEST_ROOT/SDL/SDL_image.h"
: >"$TEST_ROOT/SDL/SDL_ttf.h"
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM

for platform in desktop tg5040 tg5050; do
    case "$platform" in
    desktop)
        expected=""
        extra=-DHTTP_USE_SYSTEM_CA
        ;;
    *)
        expected="--cacert /mnt/SDCARD/.system/$platform/etc/ssl/certs/ca-certificates.crt"
        extra=''
        ;;
    esac
    cat >"$TEST_ROOT/test.c" <<EOF
#include <string.h>
#include "defines.h"
#include "http.h"
int main(void) { return strcmp(HTTP_CA_OPTION, "$expected") != 0; }
EOF
    cc -std=gnu11 -Wall -Wextra -Werror -I"$TEST_ROOT" -I"$ROOT/workspace/all/common" -I"$ROOT/workspace/$platform/platform" \
        -DPLATFORM="\"$platform\"" $extra "$TEST_ROOT/test.c" -o "$TEST_ROOT/$platform"
    "$TEST_ROOT/$platform"
done
