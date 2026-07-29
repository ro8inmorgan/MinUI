# 06 — Power, Sleep, Battery, Lid

This is the H700 power-management knowledge base: platform behavior, implementation
decisions, incident analysis, and diagnostic leads. The current test evidence and
device results are in [08-testing-status.md](08-testing-status.md#power-sleep-and-battery);
stage requirements and accepted product behavior are in
[09-roadmap.md](09-roadmap.md).

## Sleep/wake incident and resolution (2026-07-09)

The former "#1 open defect" turned out to be four separate bugs, all found and fixed:

1. **Deep sleep powered off instead of suspending.** The `suspend` script began with
   `set -uo pipefail`; the device's `/bin/sh` rejects `-o pipefail` and the script
   exited 2 *before ever writing to `/sys/power/state`*. `PWR_deepSleep` treats a
   non-zero script exit as suspend failure and escalates to `PWR_powerOff`. Fix:
   `set -u` only (comment in the script warns about this).
2. **Second and later sleeps bounced back awake (~1 s black screen).** The persistent
   `wake_fd` (a second open of event0 used only by `PLAT_shouldWake`) was never
   drained while awake, so the power-key release that *triggered* a sleep was still
   buffered and woke the device instantly. Only the first sleep of a process worked
   (fd opened lazily); launching any pak "fixed" it once by restarting nextui.elf.
   Fix: `drain_wake_fd()` runs every `PLAT_pollInput`.
3. **Lid close never slept.** Lid close injected `just_released |= BTN_SLEEP`, but the
   manual-sleep condition in `PWR_update` also requires `power_pressed_at` (a real
   power-key press), so it was a guaranteed no-op. Fix: lid close now calls the new
   `PWR_requestSleep()` (api.c), which the existing "hardware requested sleep" branch
   honors; the old injection also mis-fired on lid *open* and is gone.
4. **Keys/UI frozen ~5–10 s after wake.** Two stacked causes: (a) the script's
   `after()` (alsactl restore, syncsettings, wifi/bt restart) ran synchronously while
   the caller blocks in `system()`; (b) far worse, an ALSA PCM left open across
   suspend takes SDL ~9–10 s to close on wake (`SND_quit` inside `SND_resetAudio`).
   Fix: `PWR_enterSleep` now calls `SND_quit()` (close the healthy PCM *before*
   suspend; wake just reopens), and the script splits resume work into `after_sync`
   (alsactl/syncsettings, must finish before the script exits) and `after_async &`
   (wifi/bt/hooks). Measured wake-to-responsive: ~0.5–1.2 s (audio reinit 150–200 ms,
   formerly 9–10 s).

Physical evidence for the resolved paths—including repeated cycles, autosleep,
in-game resume, lid handling, and state restoration—is recorded in POWER-01 through
POWER-04 of the [test report](08-testing-status.md#power-sleep-and-battery). The
implementation leaves long-press power-off unchanged.

Stockmod/logind interference was ruled out: `HandlePowerKey=ignore` is in place (our
launch.sh drop-in), stockmod's `pwr_new.sh` is not running under NextUI, and no evdev
device exposes a lid switch (`sw=0` everywhere) — the hall sensor exists *only* as the
AXP2202 `hallkey` sysfs node that our code polls, so nothing else can react to it.

## Hardware facts (00 has the full list)

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
   - when the SP-only `axp2202-battery/os_sleep` attribute exists, write stock's
     persisted Super Standby value (`16`) before every attempt. Any non-zero value is
     equivalent in the vendor driver, but matching stock keeps the integration
     recognizable. This selects full USB-controller suspend and removes hall-open from
     the kernel wake set; non-SP kernels have no attribute and already use full USB
     suspend.
   - `echo mem > /sys/power/state` in a retry loop (5 tries, with the tg5040-style
     false-negative workaround: if we were asleep >5 s, a failed-looking write is
     counted as success)
   - `after_sync()` (before the script exits): `alsactl restore` mixer,
     **`syncsettings.elf`** (re-applies volume, brightness, colortemp, contrast,
     saturation, exposure **and the displaycal gamma LUT** — the panel does lose
     state across suspend). Must stay synchronous: the caller reinitializes ALSA
     as soon as `system()` returns, and a concurrent alsactl restore stalls it.
   - `after_async()` (backgrounded, `&`): restart wifi/bt services, then
     `post-resume.d` hooks — takes ~5 s and must not delay the script's exit or
     the caller's input handling freezes for that long.
   - systemd is bypassed on purpose (direct sysfs write); when logind is present,
     launch.sh best-effort installs a drop-in (`HandlePowerKey=ignore`,
     `HandlePowerKeyLongPress=ignore`) and restarts logind so it does not handle the
     power key (02)

### Wake & autosleep
- `PLAT_shouldWake`: power-key (code 116) read from a persistent non-blocking fd on
  event0; the same fd is drained every `PLAT_pollInput` while awake (see fix #2
  above); lid gate below. Generic autosleep (`PWR_*` in api.c: idle timeout, disabled
  while charging/HDMI) works unchanged on top.

## Lid (RG34XXSP)

`LID_PATH = axp2202-battery/hallkey`; `PLAT_initLid` sets `has_lid` if the file exists
(absent on RG40XXV). Polarity verified on hardware: 1 = open. Lid close →
`PWR_requestSleep()` → normal two-stage sleep (screen off, then suspend after the
suspend timeout). Lid open wakes from *screen-off* sleep (hallkey polled by
`PLAT_shouldWake`); waking from *deep* suspend requires the power key because the hall
sensor is not a kernel wake source — same as stock firmware, considered desirable.
The intended software behavior is to ignore power while the lid is closed, and
`PLAT_shouldWake()` has that gate. Hardware: POWER can still wake **light** sleep
while the lid is closed. In **deep** sleep, if POWER briefly wakes the unit and the
lid is still closed, the unit returns to sleep. Its lifecycle disposition is in the
[roadmap](09-roadmap.md#accepted-behavior-and-non-gates).

## Battery

- `PLAT_getBatteryStatus`: `axp2202-battery/capacity` + `axp2202-usb/online`, with
  coarse bucketing in shared code. H700 uses the USB `online` value directly; unlike
  tg5040 it does not also require a positive `time_to_full_now`. The recorded
  RG34XXSP evidence is in POWER-04 and POWER-05. One long-suspend observation found the
  top-right battery icon and Battery pak temporarily out of sync, despite a sensible
  reported level after a fresh boot. Investigate a stale cache or polling gap after
  long suspend.
- **Not supported:** “Keep awake over USB” (upstream #783). `PLAT_isUSBConnected()` is
  a stub returning 0; the Settings toggle remains tg5040-only. Charging already blocks
  deep sleep via `is_charging`; gadget/data keep-awake is not implemented.
- While charging, lid or power sleep reaches screen-off/light sleep only. This matches
  shared `PWR_waitForWake()` behavior: the charging check deliberately skips
  `PWR_deepSleep()` and checks again a minute later. It is a shared product behavior,
  not a failed suspend attempt or an H700-specific override; see POWER-04 and the
  [roadmap](09-roadmap.md#accepted-behavior-and-non-gates).
- Richer metrics (`time_to_empty_now`, `voltage_now`, `temp`, `charge_counter`) are
  available for future batmon extensions; not wired.
- Standby-drain investigation: the first RG34XXSP observation was a capacity change
  from 40% to 33% over about 8.5 hours of deep sleep. Capacity is coarse, and the UI
  desynchronization above may muddy the observation; it also predates the Super
  Standby `os_sleep=16` integration. Re-test with
  `axp2202-battery/voltage_now` before/after and run the same interval on stock OS
  for a baseline. POWER-06 records the evidence; the follow-up is tracked in the
  [roadmap](09-roadmap.md#future-hardening).

## Power off / reboot

Launch-loop sentinels: `/tmp/poweroff` → `poweroff_next`, `/tmp/reboot` → `reboot_next`,
each falling back to the plain busybox/systemd command if the tool exits non-zero.
Charging-while-off is handled by u-boot/stock before our hijack, untouched.

### The AXP2202 needs an explicit software power-off

The port originally shipped bare `poweroff` on the theory that "systemd poweroff works
fine". It does not. `reboot(LINUX_REBOOT_CMD_POWER_OFF)` lands in the kernel's generic
`axp20x_power_off`, which writes `AXP20X_OFF_CTRL` at **0x32** — not the power-off
register on this part. AXP2202 (same die as AXP717) moved the on/off control group the
AXP2101 keeps at 0x10 out to **0x27**, so the kernel's write is a no-op: the CPU halts,
the rails stay up, and because `PLAT_powerOff` has already killed the backlight and
blanked the framebuffer, the user sees "the screen went black but it never turned off".

`workspace/h700/poweroff_next/` fixes it, adapted from tg5040's tool (itself vendored
from Helaas's `nextui-brick-poweroff-hook`). Sequence, against the AXP2202 at 0x34:

| Write | Why |
|---|---|
| `0x40`–`0x44` ← `0x00` | Mask every IRQ source |
| `0x48`–`0x4C` ← `0xFF` | Clear pending IRQ status (write-1-to-clear) |
| `0x22` ← `0x0A` | `PWROFF_EN`: bit0=0 button event powers off (not restart), bit1 long-press, bit3 LDO-OC, bit2 die-overtemp off |
| `0x27` ← `0x01` | `SOFT_PWROFF` bit0 — the actual trigger |

The IRQ masking is likely the primary fix: a pending unmasked IRQ holds the AXP's IRQ
pin low, and >16 ms of that powers the PMU straight back on. The power-key edge IRQs
(`PONP`/`PONN` in `IRQ_EN1`) are armed by stock firmware, so the very press that asked
for power-off is the most likely thing to undo it. The `0x27` write is a plain store,
not read-modify-write, which also clears bit3 (PWROK pulled low restarts the system) —
set on stock H700 firmware, observed as `0x27 = 0x08`.

**This is not a battery disconnect.** That is `0x12` bit 3 (`BATFET_CTRL`), which the
sequence never touches — the "soft-disconnect the battery" description that circulates
for the Brick fix traces to an earlier brute-force experiment with different registers
and misattributed names. `0x12` is eFuse-defaulted and governs the battery-only
powered-off case; leave it alone.

### H700-specific divergences from the tg5040 tool

- **Bus is auto-detected**, not hardcoded. tg5040 uses `/dev/i2c-6`; H700 has the PMIC
  on `/dev/i2c-5` (`soc/twi5/i2c-5/5-0034`). The tool scans
  `/sys/bus/i2c/devices/*/name` for an `axp*` at 0x34 and falls back to `/dev/i2c-5`.
- **The card path is resolved with `realpath`.** `SDCARD_PATH` is `/mnt/SDCARD`, which
  launch.sh makes a symlink (or bind mount) onto `/mnt/sdcard`. `/proc/mounts` and
  `/proc/*/fd` both name the resolved path, so the tg5040 string comparisons against
  `/mnt/SDCARD` would never match here.
- **No global process kill.** tg5040 SIGTERM/SIGKILLs every pid before powering off.
  On the base OS `/etc/inittab` has `::respawn:/sbin/nextui-session`, so killing
  launch.sh starts a *new* frontend racing the shutdown — the exact failure being
  fixed. The PMIC cut is instantaneous and total, so nothing needs reaping first;
  `CFG_getPowerOffProtection()` gates a sync + swapoff + detach-unmount and no more.
- **`reboot_next` does no PMIC writes at all** — a restart is the SoC's job. It exists
  for the deterministic sync and signal blocking. (tg5040's launch.sh calls
  `reboot_next` but never builds or ships it, so on the Brick that line is a
  command-not-found followed by `exit 0`.)
- `--dry-run` reports the detected bus, resolved card path and planned writes, and
  touches nothing. Safe on a live device.

## CPU governor

`governor.sh` reads the available frequency list. Auto uses `schedutil` across the
full advertised range, performance uses `performance` at the greatest advertised
frequency, and powersave uses `conservative` capped mid-range. H700's advertised
1.5 GHz ceiling is in-spec rather than an overclock, so Auto no longer caps its
maximum one frequency step below that ceiling. Menu vs in-game profiles ride the
shared settings; performance is forced during game launch. Manual in-game changes
are clean.
~~RG34XXSP MD Auto CPU slowdown~~ Fixed (user-verified 2026-07-20): earlier, some
shader/scale combos left Auto near 480 MHz and felt slow; that no longer reproduces.
Manual powersave/performance were already smooth. Single cluster → no core pinning.
