# 07 — WiFi & Bluetooth

This is the H700 connectivity knowledge base: stock integration, service ownership,
design rationale, and diagnostics. Current test evidence and device results live in
[08-testing-status.md](08-testing-status.md#network-and-online-features) (with
Bluetooth audio under [Audio](08-testing-status.md#audio)); lifecycle decisions live
in [09-roadmap.md](09-roadmap.md).

## Hardware / stock OS (verified — details in 00)

- WiFi: Realtek RTL8821CS (SDIO), driver `8821cs`, `wlan0`; link state via
  `/sys/class/net/wlan0/operstate`
- BT: same combo chip, UART-attached; stock already runs `rtk_hciattach` and
  `bluetoothd` (BlueZ 5.64–5.66 depending on firmware) at boot
- Stock also runs wpa_supplicant (D-Bus mode), NetworkManager, and dnsmasq. NextUI
  explicitly stops NetworkManager and the stock supplicant before starting its own;
  the H700 scripts do not directly manage dnsmasq

## WiFi architecture (NextUI-owned wpa_supplicant)

The plan's option 1 shipped as designed: `generic_wifi.c` remains unchanged and
NextUI owns the supplicant. Evidence for scan/connect/forget, resume recovery, and
NTP synchronization is recorded in NET-01 and NET-02 of the
[test report](08-testing-status.md#network-and-online-features).

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
script stops the stack in `before()` and restarts it in `after_async()` (06). NET-02
records the resulting resume behavior.

RetroAchievements uses this connection path; its current evidence is NET-03. Pak Store
and OTA are separate product scope; their recorded status and disposition are in the
[test report](08-testing-status.md#network-and-online-features) and
[roadmap](09-roadmap.md#stretch-goals-and-out-of-scope-work).

## Bluetooth architecture (system BlueZ and stock A2DP)

- NextUI uses the stock BlueZ stack → no `btmanager`/upgrade-pakz. A clean 2026
  RG40XXV reports 5.66 for both `bluetoothd` and `bluetoothctl`; earlier firmware
  probing reported 5.64. `generic_bt.c` detects the client version at runtime.
- The stock frontend normally creates `hci0` by calling
  `/mnt/vendor/ctrl/setBluetooth.sh`, but NextUI replaces that frontend before the
  call occurs. `bt_init.sh` now owns that vendor attach/enable lifecycle, waits for
  `hci0`, and only then starts or restarts stock `bluetooth.service`.
- Bluetooth lifecycle failures are recorded in
  `$LOGS_PATH/bluetooth.txt` instead of being silently discarded.
- `HAS_BTAGENT` is enabled for H700 (the Settings pairing agent; the Jammy sysroot
  supplies glib). Settings registers it as BlueZ's default agent while the Bluetooth
  menu/pairing window is active and unregisters it after a successful pairing. The
  init script enables discoverable/pairable state but does not run a
  `bluetoothctl` agent.
- **BT audio: stock-first A2DP enabled** — the clean 2026 RG40XXV stock image has
  BlueALSA 4.2.0, its ALSA plugins/configuration, and its D-Bus policy. `bt_init.sh`
  starts `bluealsa -p a2dp-source --initial-volume=100` after BlueZ is ready and
  verifies `org.bluealsa`. It intentionally keeps BlueALSA's default volume mode;
  `--a2dp-volume` can leave AirPods at volume zero on this firmware. The H700
  `audiomon` build omits only the
  `delay 0` setting rejected by the stock plugin; its remaining behavior is shared.
  BlueALSA, its ALSA plugins, SBC, and BlueZ all remain stock and are not bundled or
  replaced. For AirPods 4 ANC, NextUI retains pairing, manual Connect selects the
  audio route, and casing the buds returns audio to the internal speaker. Auto-connect
  is not assumed because nearby paired iPhone or Mac devices may take precedence. See
  AUDIO-04 for the recorded evidence and the
  [roadmap](09-roadmap.md#accepted-behavior-and-non-gates) for the supported path.
- BT controller transport: Settings discovers, pairs, trusts, connects, and exposes a
  DualSense as an SDL joystick and evdev input device; INPUT-05 records the device
  evidence.
  Button semantics are not fully normalized because the shared tg5040/tg5050/H700
  input architecture interprets raw SDL joystick indices using platform constants.
  That is a cross-platform controller-mapping issue rather than a controller
  transport failure. INPUT-05 and INPUT-06 record the observed attachment and mapping
  results.

## Diagnostics

`PLAT_wifiDiagnosticsEnabled/Enable` + BT equivalents: tg5040 mechanism (verbose logs
under `$LOGS_PATH`) carried over. H700 has no separate keep-network flag or crash-loop
SSH rescue path. Collect logs from TF2 after poweroff or use stock SSH while the device
is already reachable.
