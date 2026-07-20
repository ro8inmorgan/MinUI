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
  desynchronization above may muddy the observation. Re-test with
  `axp2202-battery/voltage_now` before/after and run the same interval on stock OS
  for a baseline. POWER-06 records the evidence; the follow-up is tracked in the
  [roadmap](09-roadmap.md#future-hardening).

## Power off / reboot

Launch-loop sentinels: `/tmp/poweroff` → `poweroff`, `/tmp/reboot` → `reboot`.
Systemd handles clean unmounts — works; the old port's sysrq fallback
(`echo s/u/o > /proc/sysrq-trigger`) was never needed. Charging-while-off is handled
by u-boot/stock before our hijack, untouched.

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
