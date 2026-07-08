# 07 — WiFi & Bluetooth

## What the hardware/OS gives us (verified live)
- WiFi: Realtek **RTL8821CS** (SDIO), driver `8821cs`, interfaces `wlan0` (+`wlan1` virtual)
- BT: same combo chip, UART-attached (`rtk_hciattach ttyS1 rtk_h5`), `rtl_btlpm` module
- Stock services running: `wpa_supplicant` (with `-u -s` systemd/dbus mode),
  **NetworkManager**, `bluetoothd` (BlueZ, Ubuntu 22.04 = 5.64), dnsmasq
- `/sys/class/net/wlan0/operstate` for link state (what `PLAT_getNetworkStatus` reads)

## WiFi strategy

NextUI's `generic_wifi.c` speaks **`wpa_cli` to a wpa_supplicant control socket**
(`WIFI_SOCK_DIR`, tg5040: `/etc/wifi/sockets`) + `udhcpc` + `rfkill.elf`. On H700 two
options:

1. **(Recommended) Drive wpa_supplicant directly, NextUI-owned** — parity with tg5040,
   `generic_wifi.c` unchanged:
   - launch.sh: `systemctl stop NetworkManager` (and disable its autostart wants — or
     `nmcli dev set wlan0 managed no` if we prefer to leave NM for nothing), keep/own a
     wpa_supplicant instance with `-C /etc/wifi/sockets` style ctrl dir via our
     `etc/wifi/wifi_init.sh`
   - DHCP: Ubuntu has no udhcpc by default — use `dhclient` (present) or ship a tiny
     `udhcpc` (busybox) in `.system/h700/bin`; `generic_wifi.c` shells out — check
     which client name it invokes and parameterize if needed
   - `#define WIFI_SOCK_DIR "/tmp/wifi/sockets"` (writable, no rootfs touch)
2. Rewrite `PLAT_wifi*` on top of NetworkManager (`nmcli`) — less code drift from the
   OS, but diverges from generic_wifi.c and duplicates a working implementation.

Go with (1); it keeps 100% shared code and the stock OS doesn't need NM for anything
we use. Credentials store, scan lists, connect flows, diagnostics — all shared.

`rfkill.elf` (tg5040 helper) builds unchanged; Ubuntu also has `/usr/sbin/rfkill`.

### Suspend interplay (see 06)
Observed: WiFi did not recover by itself after a `mem` suspend during probing. The
suspend wrapper must bounce the link (wpa_cli suspend/resume or ifdown/up + rejoin);
measure sleep drain with the SDIO card left powered vs `rfkill block`ed pre-suspend.

## Bluetooth strategy

tg5040 uses `generic_bt.c` = `bluetoothctl` + **bluealsa** + amixer; it even ships a
BlueZ-upgrade pakz because Tina's BlueZ is ancient. On H700/Ubuntu 22.04:
- BlueZ 5.64 already on device → **no `btmanager`/upgrade-pakz needed**
- `bluetoothctl` present and version-compatible with generic_bt.c's parsing (it
  version-detects) — verify against 5.64 output formats
- **bluealsa is NOT part of Ubuntu 22.04** → build `bluez-alsa` in the toolchain and
  ship `bluealsa` + `bluealsa-aplay`/lib in `.system/h700/{bin,lib}`; our `bt_init.sh`
  starts `bluetoothd` (via systemd unit already present) + our `bluealsa --profile=a2dp-source`
- HCI bring-up: stock already runs `rtk_hciattach` at boot (confirmed running) — our
  `bt_init.sh` only needs to ensure it's up (`hciconfig hci0 up`), not re-attach
- `HAS_BTAGENT` (settings pairing agent, needs glib): jammy sysroot has glib —
  enable, gated the same way tg5040 does in `workspace/all/settings/makefile`

## Feature parity checklist (all shared UI, works once PLAT_/scripts are wired)
- [ ] toggle wifi on/off, scan, connect (open/PSK), forget, signal strength
- [ ] `PLAT_connectionStrength`, online indicator in status bar
- [ ] NTP time sync after connect (`timedatectl set-ntp true`, see 03 timezones)
- [ ] RetroAchievements + Pak Store + OTA updater (all pure-HTTP on top of wlan0 — free)
- [ ] BT controller pairing + input in nextui/minarch
- [ ] BT audio (a2dp via bluealsa) incl. `PLAT_bluetoothStream*` + samplerate limit
- [ ] airplane behavior: both radios off = measurably lower idle draw

## Diagnostics
`PLAT_wifiDiagnosticsEnabled/Enable` + BT equivalents: tg5040 toggles verbose logs —
copy mechanism (log paths under `$LOGS_PATH`).
