#!/bin/sh
set -eu

ROOT=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)

for platform in desktop tg5040 tg5050; do
    launch="$ROOT/skeleton/SYSTEM/$platform/paks/MinUI.pak/launch.sh"
    grep -F 'RUNTIME_PATH="/tmp/nextui-runtime"' "$launch" >/dev/null
    grep -F 'export NEXTUI_RUNTIME_PATH="$RUNTIME_PATH"' "$launch" >/dev/null
    grep -F 'HOOK_CMD="$LAUNCH_PROGRAM${LAUNCH_ARGUMENT:+ $LAUNCH_ARGUMENT}"' "$launch" >/dev/null
    grep -F 'NextUI: invalid launch record: $NEXT_PATH' "$launch" >/dev/null
    grep -F 'LC_ALL=C printf' "$launch" >/dev/null
    grep -F 'ACTIVE_ROM_PATH="$RUNTIME_PATH/active.rom"' "$launch" >/dev/null
    grep -F 'gametimectl.elf start "$LAUNCH_ARGUMENT"' "$launch" >/dev/null
    grep -F 'rm -f "$ACTIVE_ROM_PATH"' "$launch" >/dev/null
    if grep -Eq '(^|[[:space:]])eval([[:space:]]|$)' "$launch"; then
        echo "eval in $launch" >&2
        exit 1
    fi
done

grep -F 'getenv("NEXTUI_RUNTIME_PATH")' "$ROOT/workspace/all/nextui/nextui.c" >/dev/null
grep -F 'unlink(ACTIVE_ROM_PATH);' "$ROOT/workspace/all/nextui/nextui.c" >/dev/null
if grep -q '/tmp/next' "$ROOT/workspace/all/libgametimedb/gametimedb.c"; then
    echo 'legacy command-based gametime resume remains' >&2
    exit 1
fi
grep -F 'open(ACTIVE_ROM_PATH, O_RDONLY | O_NOFOLLOW)' "$ROOT/workspace/all/libgametimedb/gametimedb.c" >/dev/null
grep -F 'status.st_uid != geteuid() || (status.st_mode & 63) != 0' "$ROOT/workspace/all/libgametimedb/gametimedb.c" >/dev/null
sleep_body=$(sed -n '/^void PWR_sleep(void)/,/^}/p' "$ROOT/workspace/all/common/api.c")
printf '%s\n' "$sleep_body" | grep -F 'gametimectl.elf stop_all' >/dev/null
printf '%s\n' "$sleep_body" | grep -F 'gametimectl.elf resume' >/dev/null
for platform in tg5040 tg5050; do
    grep -F 'unlink("/tmp/nextui-runtime/nextui.exec");' "$ROOT/workspace/$platform/platform/platform.c" >/dev/null
done

grep -F '[ -z "${FAN_DONE:-}" ]' "$ROOT/skeleton/SYSTEM/tg5050/bin/fancontrol" >/dev/null
grep -F 'FAN_DONE=1' "$ROOT/skeleton/SYSTEM/tg5050/bin/fancontrol" >/dev/null

grep -F 'SDCARD_PATH "/.system/" PLATFORM "/etc/ssl/certs/ca-certificates.crt"' "$ROOT/workspace/all/common/http.h" >/dev/null
grep -F '#define HTTP_CA_OPTION ""' "$ROOT/workspace/all/common/http.h" >/dev/null
grep -F -- '-DHTTP_USE_SYSTEM_CA' "$ROOT/workspace/desktop/platform/makefile.env" >/dev/null
if grep -Eq 'curl .* -k([[:space:]]|$)' "$ROOT/workspace/all/common/http.c"; then
    echo 'insecure curl fallback present' >&2
    exit 1
fi
for platform in tg5040 tg5050; do
    bundle="$ROOT/skeleton/SYSTEM/$platform/etc/ssl/certs/ca-certificates.crt"
    [ -s "$bundle" ]
    openssl crl2pkcs7 -nocrl -certfile "$bundle" | openssl pkcs7 -print_certs -noout | grep -q '^subject='
done
[ -s "$ROOT/skeleton/SYSTEM/CA-BUNDLE-PROVENANCE.txt" ]
[ -s "$ROOT/skeleton/SYSTEM/CA-BUNDLE-MPL-2.0.txt" ]
grep -F '} hints[3];' "$ROOT/workspace/all/common/api.c" >/dev/null
grep -F 'for (int i = 0; i < 3; i++)' "$ROOT/workspace/all/common/api.c" >/dev/null
grep -F 'if (ftruncate(fd, 0) != 0)' "$ROOT/workspace/all/common/config.c" >/dev/null
for menu in wifimenu btmenu; do
    grep -F 'ScanSnapshot' "$ROOT/workspace/all/settings/$menu.hpp" >/dev/null
    grep -F 'snapshotMutex' "$ROOT/workspace/all/settings/$menu.cpp" >/dev/null
    if sed -n '/^void Menu::updater()/,/^}/p' "$ROOT/workspace/all/settings/$menu.cpp" | grep -q 'items'; then
        echo "worker mutates menu items: $menu" >&2
        exit 1
    fi
done
