#!/bin/sh
set -eu

build_dir=${BUILD_DIR:-/tmp/nextui-tests}
sanitizer=${SANITIZER:-address,undefined}
mkdir -p "$build_dir/SDL"
: >"$build_dir/SDL/SDL.h"
: >"$build_dir/SDL/SDL_image.h"
: >"$build_dir/SDL/SDL_ttf.h"
cc -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -fsanitize="$sanitizer" -fno-omit-frame-pointer \
    -I"$build_dir" -I../workspace/all/common -I../workspace/desktop/platform -DPLATFORM='"desktop"' \
    -c ../workspace/all/common/utils.c -o "$build_dir/utils.o"
cc -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -Wformat=2 -Werror -fsanitize="$sanitizer" -fno-omit-frame-pointer \
    -I"$build_dir" -I../workspace/all/common -I../workspace/all/minarch -I../workspace/desktop/platform \
    -DPLATFORM='"desktop"' path_helpers_test.c ../workspace/all/common/path_helpers.c \
    ../workspace/all/minarch/archive_extract.c "$build_dir/utils.o" -lm -o "$build_dir/path_helpers_test"
ASAN_OPTIONS=detect_leaks=1 "$build_dir/path_helpers_test"
./p1_hardening_test.sh
./manifest_test.sh
./make_link_test.sh
./core_artifact_test.sh
python3 ./locked_checkout_test.py
./runtime_contract_test.sh
./ca_bundle_contract_test.sh
./deterministic_zip_test.sh
