# 06 — Power, Sleep, Battery, Lid

This is a headline goal of the port: **match the RG XX stock OS's excellent sleep
behavior**. Good news first — verified live on the RG40XXV:

- `/sys/power/state` = `freeze mem` — same suspend-to-RAM support as TG5040
- `echo mem`-style suspend **works** (tested via `rtcwake -m mem`); draw in `mem`
  suspend on these devices is famously low (days of standby) because the stock kernel
  properly powers down WiFi/panel/SoC
- Wake source: **power button** (AXP2202 PEK is a wakeup source — `axp2202-battery`
  and `axp2202-usb` are active wakeup_sources). RTC alarm wake did NOT fire in
  testing → no timed wake; don't build features on it.

## Sleep design (mirror tg5040, simplify where the OS is nicer)

NextUI has two layers; keep both:

1. **Screen-off "light sleep"** (`PLAT_enableBacklight(0)` + governor powersave):
   - backlight off = raw brightness 0 via `/dev/disp` ioctl + fb blank
     (`/sys/class/graphics/fb0/blank` ← old port did both; keep both)
   - LED signal: `work_led` (0=on 1=off) + honor `workled_sleep` for behavior in sleep
2. **Deep sleep** (`PLAT_supportsDeepSleep()=1` → generic `PLAT_deepSleep()` writes
   `mem` to `/sys/power/state`, with retry loop — reuse as-is), wrapped by the
   `skeleton/SYSTEM/h700/bin/suspend` script pattern from tg5040:
   - pre-suspend: save ALSA state, `wpa_cli suspend` / stop wpa_supplicant + bluetoothd
     (or `rfkill block` — measure which yields lowest draw), run `pre-sleep.d` hooks
   - `echo mem > /sys/power/state` (with tg5040's false-negative retry workaround)
   - post-resume: restore ALSA, restart wifi (**required** — observed in probing: after
     resume WiFi did not come back on its own), re-apply brightness + displaycal LUT
     if the panel re-init clears the gamma table (test!), run `post-resume.d` hooks
   - systemd note: we bypass `systemctl suspend` on purpose (direct sysfs write, like
     stock does); make sure systemd-logind doesn't also react to the power key —
     `HandlePowerKey=ignore` in logind.conf, or mask logind's key handling at launch
     (check stock config first: the stock UI also handles the key itself, so stock
     firmware has likely already neutered logind — verify `loginctl show-logind`).

### Wake & autosleep
- `PLAT_shouldWake`: power-key event (code 116 on event0) — copy old port logic,
  including the lid gate (below).
- Generic autosleep (`PWR_*` in api.c: idle timeout, disabled while charging/HDMI)
  works unchanged once PLAT hooks are right.

## Lid (RG34XXSP — clamshell)
- `LID_PATH = /sys/class/power_supply/axp2202-battery/hallkey` (old port; confirm the
  file exists on the SP when in hand — absent on RG40XXV probes so far... it appears
  only on units with the hall sensor, `PLAT_initLid` sets `has_lid = exists(LID_PATH)`).
- Behavior: lid close → inject `BTN_SLEEP` (→ deep sleep); while closed, swallow
  power-key wake (only lid-open wakes) — this is exactly the old port's
  `PLAT_shouldWake`/`PLAT_pollInput` logic; port it into the current
  `PLAT_initLid`/`PLAT_lidChanged` API (fallbacks currently no-op).
- Check whether hall-close already triggers a kernel-level suspend on stock (some
  firmwares wire it in-kernel); if so, coordinate rather than double-handle.

## Battery
- `PLAT_getBatteryStatus(Fine)`: `axp2202-battery/capacity` (0-100) +
  `axp2202-usb/online` — same paths as tg5040, copy verbatim; coarse bucketing in
  shared code.
- Bonus (nice-to-have): H700 exposes `time_to_empty_now`, `time_to_full_now`,
  `voltage_now`, `temp`, `charge_counter` — batmon's SQLite logging gets richer data
  for free if we wire `PLAT_` extensions later; not required for parity.
- Low battery: shared code handles warnings; verify `capacity_alert_min` interplay
  doesn't force-poweroff underneath us (stock `cexpert`/MCU behaviors — we kill
  stock daemons at launch, see 02).

## Power off / reboot
- launch loop: `/tmp/poweroff` → `poweroff`, `/tmp/reboot` → `reboot` (systemd).
  Verify clean unmount of `/mnt/sdcard` (vfat!) — systemd handles mounts it knows;
  if we manually mounted TF2, unmount in the shutdown path (or add a mount unit).
  If systemd poweroff is slow/unreliable from our context, port tg5040's
  `poweroff_next` approach (sync + sysrq or direct PMIC) — the old port used
  `shutdown` script + sysrq (`echo s/u/o > /proc/sysrq-trigger`), which is proven on
  this hardware and immune to systemd state.
- Charging-while-off: stock shows a charge animation via `charg.dge` (from
  launcher.sh). When powered off and plugged in, u-boot/stock handles it before our
  hijack — expected to work untouched; verify `bootreason=button` vs charger paths.

## CPU governor / performance profiles
- `governor.sh` clone: auto=`schedutil` (available ✔), performance=`performance`
  (max 1512000), powersave=`conservative` capped ~720k. Same policy0 sysfs as tg5040.
- Menu/UI: powersave or schedutil; in-game: per-NextUI settings (auto/performance) —
  all shared code once governor.sh maps correctly.
- H700 is a single A53 cluster: `PLAT_pinToCores` = no-op.

## Standby-drain acceptance test (09 has the full matrix)
Overnight test: 8h in deep sleep, battery delta ≤ ~3-4% (stock-comparable). Measure
stock first on the same unit for a baseline, then NextUI sleep, then iterate on what
to shut down pre-suspend (WiFi off vs powered, BT hci down, etc.).
