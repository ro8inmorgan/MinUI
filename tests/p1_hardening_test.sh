#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
TEST_ROOT=$(mktemp -d "$ROOT/tests/.p1.XXXXXX")
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM

make_payload() {
    payload=$1
    platform=$2
    rm -rf "$payload"
    mkdir -p "$payload/.system/$platform/bin" "$payload/.system/$platform/paks/MinUI.pak" "$payload/.tmp_update" "$payload/Tools/$platform"
    : >"$payload/.system/$platform/bin/nextui.elf"
    printf '#!/bin/sh\nexit 0\n' >"$payload/.system/$platform/paks/MinUI.pak/launch.sh"
    printf '#!/bin/sh\nexit 0\n' >"$payload/.tmp_update/$platform.sh"
    printf '#!/bin/sh\n: > "$NEXTUI_RECOVERY_PROOF"\n' >"$payload/.tmp_update/updater"
    : >"$payload/Tools/$platform/tool"
    chmod 755 "$payload/.system/$platform/paks/MinUI.pak/launch.sh" "$payload/.tmp_update/$platform.sh" "$payload/.tmp_update/updater"
    if [ "${FAIL_POST:-0}" = 1 ]; then
        printf '#!/bin/sh\nexit 1\n' >"$payload/.system/$platform/bin/install.sh"
        chmod 755 "$payload/.system/$platform/bin/install.sh"
    fi
    (cd "$payload" && find .system .tmp_update Tools -type f -print | sort | xargs sha256sum >SHA256SUMS)
}

fake_unzip="$TEST_ROOT/unzip"
cat >"$fake_unzip" <<'EOF'
#!/bin/sh
case "$1" in
-t) [ "${FAKE_UNZIP_FAIL_TEST:-0}" != 1 ] ;;
-Z1) (cd "$FAKE_PAYLOAD" && find . -print) ;;
-o) destination=""; shift; archive=$1; shift
    while [ "$#" -gt 0 ]; do
        if [ "$1" = -d ]; then destination=$2; shift 2; else shift; fi
    done
    cp -R "$FAKE_PAYLOAD"/. "$destination" ;;
*) exit 1 ;;
esac
EOF
chmod 755 "$fake_unzip"

for platform in tg5040 tg5050; do
    sd="$TEST_ROOT/$platform"
    payload="$TEST_ROOT/payload-$platform"
    mkdir -p "$sd/.system/$platform/bin" "$sd/.tmp_update"
    printf old >"$sd/.system/$platform/bin/runtime"
    printf old >"$sd/.tmp_update/$platform.sh"
    : >"$sd/MinUI.zip"
    make_payload "$payload" "$platform"

    if NEXTUI_TEST=1 SDCARD_PATH="$sd" NEXTUI_UNZIP="$fake_unzip" FAKE_PAYLOAD="$payload" FAKE_UNZIP_FAIL_TEST=1 sh "$ROOT/workspace/$platform/install/boot.sh"; then
        echo "corrupt update unexpectedly succeeded" >&2
        exit 1
    fi
    [ "$(cat "$sd/.system/$platform/bin/runtime")" = old ]
    [ -f "$sd/MinUI.zip" ]
    [ -f "$sd/.nextui-update-failed" ]
    [ ! -e "$sd/.nextui-update-state" ]

    FAIL_POST=1 make_payload "$payload" "$platform"
    if NEXTUI_TEST=1 SDCARD_PATH="$sd" NEXTUI_UNZIP="$fake_unzip" FAKE_PAYLOAD="$payload" sh "$ROOT/workspace/$platform/install/boot.sh"; then
        echo "failed post-install unexpectedly succeeded" >&2
        exit 1
    fi
    [ "$(cat "$sd/.system/$platform/bin/runtime")" = old ]
    [ -f "$sd/MinUI.zip" ]

    FAIL_POST=0 make_payload "$payload" "$platform"
    NEXTUI_TEST=1 SDCARD_PATH="$sd" NEXTUI_UNZIP="$fake_unzip" FAKE_PAYLOAD="$payload" sh "$ROOT/workspace/$platform/install/boot.sh"
    [ ! -f "$sd/MinUI.zip" ]
    [ -f "$sd/.system/$platform/paks/MinUI.pak/launch.sh" ]

    for boundary in transaction-state backup-created archive-test extraction checksum prepared system-backup system-installed updater-backup updater-installed tools-backup tools-installed; do
        rm -rf "$sd"
        mkdir -p "$sd/.system/$platform/bin" "$sd/.tmp_update"
        printf old >"$sd/.system/$platform/bin/runtime"
        printf '#!/bin/sh\n: > "$NEXTUI_RECOVERY_PROOF"\n' >"$sd/.tmp_update/updater"
        chmod 755 "$sd/.tmp_update/updater"
        : >"$sd/MinUI.zip"
        make_payload "$payload" "$platform"
        set +e
        NEXTUI_TEST=1 NEXTUI_TEST_KILL_AFTER="$boundary" SDCARD_PATH="$sd" NEXTUI_UNZIP="$fake_unzip" FAKE_PAYLOAD="$payload" sh "$ROOT/workspace/$platform/install/boot.sh"
        status=$?
        set -e
        [ "$status" -eq 137 ]
        [ -f "$sd/MinUI.zip" ]
        [ -f "$sd/.nextui-update-state" ]
        proof="$TEST_ROOT/$platform-$boundary-recovered"
        NEXTUI_TEST=1 NEXTUI_RECOVERY_PROOF="$proof" SDCARD_PATH="$sd" sh "$ROOT/skeleton/BOOT/trimui/app/runtrimui.sh"
        [ -f "$proof" ]
        [ -f "$sd/MinUI.zip" ]
        [ ! -e "$sd/.nextui-update-state" ]
        [ ! -e "$sd/.nextui-update-backup" ]
        NEXTUI_TEST=1 SDCARD_PATH="$sd" NEXTUI_UNZIP="$fake_unzip" FAKE_PAYLOAD="$payload" sh "$ROOT/workspace/$platform/install/boot.sh"
        [ ! -f "$sd/MinUI.zip" ]
        [ -f "$sd/.system/$platform/paks/MinUI.pak/launch.sh" ]
    done

    mkdir "$sd/.nextui-update-backup"
    proof="$TEST_ROOT/$platform-orphan-backup-recovered"
    NEXTUI_TEST=1 NEXTUI_RECOVERY_PROOF="$proof" SDCARD_PATH="$sd" sh "$ROOT/skeleton/BOOT/trimui/app/runtrimui.sh"
    [ -f "$proof" ]
    [ ! -e "$sd/.nextui-update-backup" ]
done

RUNTIME="$TEST_ROOT/runtime"
mkdir -p "$RUNTIME"
NEXT_PATH="$RUNTIME/launch.argv"
launcher="$TEST_ROOT/launch tool's"
result="$TEST_ROOT/result"
# shellcheck disable=SC2016 # generated launcher must retain its positional parameters.
printf '#!/bin/sh\nprintf "%%s" "$1" > "$2"\n' >"$launcher"
chmod 755 "$launcher"
for platform in desktop tg5040 tg5050; do
    # Source the production parser only; launch execution below uses its parsed argv directly.
    sed -n '/^parse_launch_record()/,/^}/p' "$ROOT/skeleton/SYSTEM/$platform/paks/MinUI.pak/launch.sh" >"$TEST_ROOT/parser.sh"
    # shellcheck disable=SC1091 # parser is extracted from the production launch script.
    . "$TEST_ROOT/parser.sh"
    rom="$TEST_ROOT/Pokémon-$platform.gb"
    launcher_len=$(LC_ALL=C printf '%s' "$launcher" | wc -c)
    rom_len=$(LC_ALL=C printf '%s' "$rom" | wc -c)
    printf 'NEXTUI_ARGV_V1\n2\n%s\n%s\n%s\n%s\n' "$launcher_len" "$launcher" "$rom_len" "$rom" >"$NEXT_PATH"
    chmod 600 "$NEXT_PATH"
    LC_ALL=C.UTF-8 parse_launch_record
    "$LAUNCH_PROGRAM" "$LAUNCH_ARGUMENT" "$result"
    [ "$(cat "$result")" = "$rom" ]
    printf 'tampered\n1\n1\nx\n' >"$NEXT_PATH"
    if parse_launch_record; then
        echo "tampered launch record accepted" >&2
        exit 1
    fi
    head -c 8193 /dev/zero >"$NEXT_PATH"
    if parse_launch_record; then
        echo "overlong launch record accepted" >&2
        exit 1
    fi
done

cc -D_GNU_SOURCE -std=gnu11 -Wall -Wextra -Werror -I"$ROOT/workspace/all/common" \
    "$ROOT/tests/settings_atomic_test.c" -o "$TEST_ROOT/settings_atomic_test"
"$TEST_ROOT/settings_atomic_test" "$TEST_ROOT/msettings.bin"

echo "p1 hardening tests passed"
