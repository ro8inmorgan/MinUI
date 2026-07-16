# 07 — WiFi & Bluetooth

## Hardware / stock OS (verified — details in 00)

- WiFi: Realtek RTL8821CS (SDIO), driver `8821cs`, `wlan0`; link state via
  `/sys/class/net/wlan0/operstate`
- BT: same combo chip, UART-attached; stock already runs `rtk_hciattach` and
  `bluetoothd` (BlueZ 5.64) at boot
- Stock also runs wpa_supplicant (dbus mode) + NetworkManager + dnsmasq — all
  displaced at launch

## WiFi (as shipped — NextUI-owned wpa_supplicant, tested ✅)

The plan's option 1 shipped as designed: `generic_wifi.c` unchanged, NextUI owns the
supplicant. Scan/connect/forget work; the link survives sleep (bounced by the suspend
script) and NTP sync works after connect.

`skeleton/SYSTEM/h700/etc/wifi/wifi_init.sh`:
- launch.sh stops NetworkManager; wifi_init owns a wpa_supplicant instance with the
  control socket dir `WIFI_SOCK_DIR = /tmp/wifi/sockets`
- **Credentials persist**: `wpa_supplicant.conf` lives at
  `$USERDATA_PATH/wifi/wpa_supplicant.conf` (on the SD card, `update_config=1`) — an
  early version kept it in tmpfs and lost networks on reboot
- **DHCP**: Ubuntu has no udhcpc, so `dhclient` (udhcpc as fallback if someone ships
  it). Re-association is handled by a generated `wpa_action` script registered via
  `wpa_cli -a`: on `CONNECTED` it does `dhclient -r` + `dhclient -nw`, so switching
  networks re-DISCOVERs instead of keeping a stale lease
- Teardown uses `systemctl stop wpa_supplicant …` *and* `killall` belt-and-braces
- `rfkill`: h700 builds its own minimal `/dev/rfkill` ioctl tool (03)

Suspend interplay: WiFi does not survive `mem` suspend on its own; the suspend
script stops the stack in `before()` and restarts it in `after()` (06). Verified
working across sleep on RG40XXV.

RetroAchievements login and unlock work on RG34XXSP, including credential entry through
the on-screen keyboard. Pak Store and OTA update are explicitly excluded from the alpha
scope; testing is not applicable and they are not alpha release gates.

## Bluetooth (as shipped — system BlueZ, controller transport and stock A2DP enabled)

- NextUI uses the stock BlueZ stack → no `btmanager`/upgrade-pakz. A clean 2026
  RG40XXV reports 5.66 for both `bluetoothd` and `bluetoothctl`; earlier firmware
  probing reported 5.64. `generic_bt.c` detects the client version at runtime.
- The stock frontend normally creates `hci0` by calling
  `/mnt/vendor/ctrl/setBluetooth.sh`, but NextUI replaces that frontend before the
  call occurs. `bt_init.sh` now owns that vendor attach/enable lifecycle, waits for
  `hci0`, and only then starts or restarts stock `bluetooth.service`.
- Bluetooth lifecycle failures are recorded in
  `$LOGS_PATH/bluetooth.txt` instead of being silently discarded.
- `HAS_BTAGENT` enabled for h700 (settings pairing agent; jammy sysroot has glib —
  links fine, loads on device). It is the persistent pairing agent and owns the
  pairable window; the init script does not create a short-lived `bluetoothctl`
  agent.
- **BT audio: stock-first A2DP enabled** — the clean 2026 RG40XXV stock image has
  BlueALSA 4.2.0, its ALSA plugins/configuration, and its D-Bus policy. `bt_init.sh`
  starts `bluealsa -p a2dp-source --a2dp-volume --initial-volume=100` after BlueZ is
  ready and verifies `org.bluealsa`. The H700 `audiomon` build omits only the
  `delay 0` setting rejected by the stock plugin; its remaining behavior is shared.
  BlueALSA, its ALSA plugins, SBC, and BlueZ all remain stock and are not bundled or
  replaced. AirPods 4 ANC playback is verified; automatic reconnect and suspend/resume
  are tracked in 05/08.
- BT controller transport is verified on RG40XXV: Settings discovers, pairs, trusts,
  connects, and exposes a DualSense as an SDL joystick and evdev input device.
  Button semantics are not fully normalized because the shared tg5040/tg5050/H700
  input architecture interprets raw SDL joystick indices using platform constants.
  That is a cross-platform controller-mapping issue, not an H700 Bluetooth bring-up
  gap, and is deliberately deferred from this branch.

## Diagnostics

`PLAT_wifiDiagnosticsEnabled/Enable` + BT equivalents: tg5040 mechanism (verbose logs
under `$LOGS_PATH`) carried over. H700 intentionally does not add its own keep-network
flag or crash-loop SSH rescue path for alpha; collect logs from TF2 after poweroff or
use stock SSH while the device is already reachable.
