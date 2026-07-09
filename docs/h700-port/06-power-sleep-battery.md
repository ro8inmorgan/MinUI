# 06 — Power, Sleep, Battery, Lid

## ⚠️ Known issue: wake is unreliable

Deep sleep is implemented end-to-end, but on RG40XXV **the device sometimes hangs
entering or leaving suspend** (fails to wake, or freezes on the way down). This is the
port's #1 open defect. What is *known good*: when a sleep/wake cycle succeeds, state
restores correctly (display settings incl. displaycal survive sleep — tested ✅) and
WiFi reconnects (tested ✅). Debugging leads, roughly in order of suspicion:

- The kernel's own suspend reliability with our device population of drivers still up
  (stock stops different services than we do) — compare the exact pre-suspend state
  vs stock's sleep path.
- The `suspend` script's false-negative retry loop (5 tries, "asleep >5 s ⇒ treat
  write failure as success" heuristic) — a retry storm around a half-suspended SoC
  could itself wedge the device.
- `hallkey`/PEK event handling racing the suspend write (persistent `wake_fd` drains
  event0; verify no wake press is consumed *during* the transition).
- Serial/UART console (if accessible) or persisting a pre/post-suspend breadcrumb log
  to /tmp would localize hang-on-entry vs hang-on-exit — currently unknown which side
  fails.

Not suspects anymore (all verified fixed/in place): logind power-key handling
(drop-in sets `HandlePowerKey=ignore`), backgrounded resume restore (now synchronous),
per-poll event0 fd churn (persistent fd), post-resume display re-apply
(syncsettings.elf runs in `after()`).

## Verified hardware facts (00 has the full list)

- `/sys/power/state` = `freeze mem`; `echo mem` suspend works
- Wake source: **power button only** (AXP2202 PEK). RTC alarm wake does not fire.
- WiFi does not recover on its own after resume — must be bounced (07).

## Sleep design (as shipped)

Two layers, as on tg5040:

1. **Screen-off light sleep** — `PLAT_enableBacklight(0)`: raw brightness 0 via
   `/dev/disp` ioctl + fb blank (`/sys/class/graphics/fb0/blank`) + `work_led` off;
   governor powersave.
2. **Deep sleep** — `PLAT_supportsDeepSleep()=1`; generic `PLAT_deepSleep()` invokes
   `skeleton/SYSTEM/h700/bin/suspend`:
   - `before()`: `alsactl store` mixer state, stop bluetoothd/wpa_supplicant,
     `pre-sleep.d` hooks
   - `echo mem > /sys/power/state` in a retry loop (5 tries, with the tg5040-style
     false-negative workaround: if we were asleep >5 s, a failed-looking write is
     counted as success)
   - `after()` (runs **synchronously**): `alsactl restore` mixer,
     **`syncsettings.elf`** (re-applies volume, brightness, colortemp, contrast,
     saturation, exposure **and the displaycal gamma LUT** — the panel does lose
     state across suspend), restart wifi/bt services, then `post-resume.d` hooks
     (hooks themselves backgrounded, by design)
   - systemd is bypassed on purpose (direct sysfs write); launch.sh installs a logind
     drop-in (`HandlePowerKey=ignore`, `HandlePowerKeyLongPress=ignore`) and restarts
     logind so systemd never handles the power key (02)

### Wake & autosleep
- `PLAT_shouldWake`: power-key (code 116) read from a persistent non-blocking fd on
  event0; lid gate below. Generic autosleep (`PWR_*` in api.c: idle timeout, disabled
  while charging/HDMI) works unchanged on top.

## Lid (RG34XXSP) — implemented, untested

`LID_PATH = axp2202-battery/hallkey`; `PLAT_initLid` sets `has_lid` if the file exists
(absent on RG40XXV, so the code has *never run against real hardware*).
Lid close → sleep; while closed, power-key wake is swallowed (only lid-open wakes).
To verify on first RG34XXSP contact: hallkey polarity/values, whether the kernel
already suspends on hall-close by itself (would double-handle), and power-key-while-
closed behavior.

## Battery

- `PLAT_getBatteryStatus`: `axp2202-battery/capacity` + `axp2202-usb/online`, coarse
  bucketing in shared code — same paths as tg5040, copied verbatim. **Accuracy vs
  stock reading, charging indicator, and charge-while-sleeping: untested.**
- Richer metrics (`time_to_empty_now`, `voltage_now`, `temp`, `charge_counter`) are
  available for future batmon extensions; not wired.
- Standby drain: never measured (plan target was ≤ stock + ~1% over 8 h; blocked on
  the wake-reliability issue making soak tests moot).

## Power off / reboot

Launch-loop sentinels: `/tmp/poweroff` → `poweroff`, `/tmp/reboot` → `reboot`.
Systemd handles clean unmounts — works; the old port's sysrq fallback
(`echo s/u/o > /proc/sysrq-trigger`) was never needed. Charging-while-off is handled
by u-boot/stock before our hijack, untouched.

## CPU governor

`governor.sh`: auto=`schedutil`, performance=`performance` @1512000,
powersave=`conservative` capped mid-range. Menu vs in-game profiles ride the shared
settings; performance is forced during game launch. Single cluster → no core pinning.
