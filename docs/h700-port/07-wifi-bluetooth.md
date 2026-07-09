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

Untested so far: RetroAchievements login/unlock, Pak Store install, OTA update flow
(all pure HTTP on top of a working wlan0 — expected free, not yet exercised).

## Bluetooth (as shipped — system BlueZ, input yes, audio gated off)

- BlueZ 5.64 on device → no `btmanager`/upgrade-pakz (as planned); `generic_bt.c`
  drives `bluetoothctl`
- `bt_init.sh` ensures hci is up (stock's `rtk_hciattach` already ran), starts a
  `bluetoothctl` agent (NoInputNoOutput), and would start `bluealsa
  --profile=a2dp-source` **if the binary existed**
- `HAS_BTAGENT` enabled for h700 (settings pairing agent; jammy sysroot has glib —
  links fine, loads on device)
- **BT audio: deliberately not shipped this beta** — bluez-alsa is not in Ubuntu
  22.04 and we chose to gate rather than build it: `-DNO_BT_AUDIO` hides the
  samplerate UI, audiomon refuses A2DP sinks without a bluealsa binary, README says
  so. Full story + re-enable path in 05 and 09-roadmap.
- BT controller pairing/input: implemented via the SDL joystick path
  (`SDL_JOYSTICK_DISABLE_UDEV=1`; corrected JOY_* indices in 03) — **untested**.

## Diagnostics

`PLAT_wifiDiagnosticsEnabled/Enable` + BT equivalents: tg5040 mechanism (verbose logs
under `$LOGS_PATH`) carried over. H700 intentionally does not add its own keep-network
flag or crash-loop SSH rescue path for alpha; collect logs from TF2 after poweroff or
use stock SSH while the device is already reachable.
