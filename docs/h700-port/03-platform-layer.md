# 03 — Platform Layer (`workspace/h700/`)

The port's center of gravity. Strategy: **clone `workspace/tg5040/` → `workspace/h700/`**
(keeps the `generic_video.c`/`generic_wifi.c`/`generic_bt.c` includes and the full
`PLAT_*` wiring), then repoint every hardware access using the old rg35xxplus paths
(all verified live in 00-device-facts). The old code is at `git show 8cd78866:workspace/rg35xxplus/...`
— use it as the H700 hardware reference, but keep the *current* NextUI API surface
(the old code predates ~2 years of NextUI API growth: shaders, LEDs API, turbo,
timezones, displaycal, deep sleep…).

## platform.h

```c
// h700
extern int is_rg28xx, is_rg34xx, is_cube;   // set in PLAT_initVideo from DEVICE env

#define FIXED_SCALE   2
#define FIXED_WIDTH   (is_cube?720:(is_rg34xx?720:640))
#define FIXED_HEIGHT  (is_cube?720:480)
#define FIXED_BPP     2
#define SCREEN_FPS    60.0            // measure per panel! old port used 60.0; verify
                                      // with vsync timing test on each device
#define SDCARD_PATH   "/mnt/SDCARD"   // see note below
#define MAX_LIGHTS    0               // no RGB LEDs on RG XX (work_led is on/off only)
#define MUTE_VOLUME_RAW 0
#define MAIN_ROW_COUNT (is_cube?8:6)  // 480px tall screens fit 6 rows (tg5040 is 720/768px)
```

Path note: tg5040 uses `/mnt/SDCARD`; H700 mounts TF2 at `/mnt/sdcard`. Simplest 1:1:
launch.sh does `ln -s /mnt/sdcard /mnt/SDCARD` (or bind-mount) so every hardcoded
`/mnt/SDCARD` in shared code keeps working. (Grep `workspace/all` for `/mnt/SDCARD`
literals before deciding to diverge — the symlink is the low-risk move.)

### Input mappings
The kernel exposes `ANBERNIC-keys` as **both** kbd (evdev KEY_*) and joystick. Current
NextUI reads input via SDL (PLAT_updateInput/SDL events + JOY_*/AXIS_* from platform.h).
Two options:
1. **SDL joystick route (tg5040-style, recommended):** our custom SDL2 sees js0. The
   JOY_* button indices follow the kernel's KEY-bit order for gpio-keys devices —
   discover once per device family by running `minput.elf`/`evtest` on the device and
   fill platform.h. AXIS_LX/LY/RX/RY = SDL axes 0..3 (kernel ABS_Z/RX/RY/RZ map in
   order), triggers: none analog (L2/R2 are digital keys 314/315).
2. **evdev route (old-port-style):** define `BUTTON_*` = raw codes (103/108/105/106,
   304/305/307/306, 311/310/312, 308/309/314/315/313/316, 115/114/116) and implement
   `PLAT_pollInput` reading event0/1/2 directly, as the old platform.c did.

Start with (1); the codes from (2) are the fallback and are needed for keymon anyway.
`CODE_POWER 116` comes from `axp2202-pek` (event0) — note tg5040 uses 102; H700 power
key emits KEY_POWER=116 per old port. Sticks (RG40XXV, cube only): raw 0..4096,
old scaling `value*32767/4096` (SDL route handles scaling itself; verify centering/deadzone).

Buttons per device: RG40XXV has dual sticks + L3/R3; RG34XXSP/RG28XX have no sticks
(JOY_L3/R3 = JOY_NA via `is_*` conditionals, same trick as tg5040's `is_brick`).

```c
#define BTN_RESUME         BTN_X
#define BTN_SLEEP          BTN_POWER
#define BTN_WAKE           BTN_POWER
#define BTN_MOD_VOLUME     BTN_NONE
#define BTN_MOD_BRIGHTNESS BTN_MENU     // MENU+vol keys = brightness (old-port behavior)
#define BTN_MOD_COLORTEMP  BTN_SELECT   // parity with tg5040
#define BTN_MOD_PLUS       BTN_PLUS     // dedicated vol keys 115/114
#define BTN_MOD_MINUS      BTN_MINUS
```

## platform.c — subsystem by subsystem

| Subsystem | tg5040 does | h700 change |
|---|---|---|
| `PLAT_initVideo` etc. | `#include "generic_video.c"` | keep (see 04 for SDL/GLES groundwork + rotation) |
| Battery | `axp2202-battery/capacity`, `axp2202-usb/online` | **identical paths** — same PMIC family. Copy as-is. |
| CPU speed | `governor.sh` via cpufreq policy0 | same sysfs layout confirmed; freq table 480k–1512000; PERF=1512000, powersave=conservative/720k. Single cluster → `PLAT_pinToCores` = no-op. |
| CPU/GPU temp | thermal zones | enumerate `/sys/class/thermal/thermal_zone*/type` on device; wire what exists, return 0 otherwise |
| GPU speed/usage | sunxi devfreq sysfs | check `/sys/class/devfreq/` for gpu node (H700 BSP usually `1800000.gpu`); stub if absent |
| Rumble `PLAT_setRumble` | gpio227 + `/sys/class/motor/voltage` | `echo 1/0 > /sys/class/power_supply/axp2202-battery/moto`. On/off only (no voltage control) — map strength>0 → 1. Skip when HDMI (old-port behavior). Later: try input-FF via event1 (device advertises FF bits) for strength levels. |
| LEDs `PLAT_initLeds` | `/sys/class/led_anim/*` (Brick RGB) | `MAX_LIGHTS 0`; drop ledcontrol.elf from build. Keep `work_led` for sleep indication (06). |
| `PLAT_powerOff` | haptics, mute, blank, `/tmp/poweroff` | same, then systemd `poweroff` in launch.sh (verify; else port poweroff_next) |
| Lid | none (fallback no-op) | **implement for RG34XXSP**: `LID_PATH=/sys/class/power_supply/axp2202-battery/hallkey`; `PLAT_initLid` sets `has_lid` if file exists; `PLAT_lidChanged` polls it; lid-close → `BTN_SLEEP` inject, lid state gates wake (copy old port logic into current API: `PLAT_shouldWake` swallowing power-key while closed) |
| `PLAT_getModel` | TrimUI sniff | `"Anbernic " + getenv("RGXX_MODEL")` |
| `PLAT_getOsVersionInfo` | Tina version | read `/etc/os-release` PRETTY_NAME + kernel |
| Turbo (`trimui_inputd`) | trimui daemon ioctl | stub `PLAT_canTurbo()=false` initially (NextUI turbo feature — check if generic fallback exists; if the API is generic enough, enable later) |
| Timezones/NTP | OpenWRT `uci` | Ubuntu equivalents: `timedatectl set-timezone` / `list-timezones`, NTP = `timedatectl set-ntp` (systemd-timesyncd present). Much cleaner than tg5040. |
| WiFi/BT | generic_wifi/bt + init scripts | keep generic, adapt scripts (07) |
| `PLAT_pickSampleRate` | clamp + BT limit | keep |
| `PLAT_deepSleep` | generic `echo mem` | keep — verified working on device (06) |

## libmsettings (`workspace/h700/libmsettings/msettings.c`)

Clone tg5040's, repoint:
- **Brightness**: `SetRawBrightness(0-255)` → `/dev/disp` ioctl `DISP_LCD_SET_BRIGHTNESS (0x102)`,
  args `{0, val, 0, 0}` — same driver as tg5040, same code. Reuse old port's level
  curve (`0→4, 1→6, 2→10, 3→16, 4→32, 5→48, 6→64, 7→96, 8→128, 9→192, 10→255`).
  Kill stock `brightCtrl.bin` at launch so it doesn't fight us.
- **Volume**: tinyalsa mixer on card0; controls: `digital volume` (master) and
  `lineout volume` — map NextUI 0..20 scale; discover ranges via `amixer cget`.
  Confirm speaker/HP switching: `SPK Switch` + jack events (below).
- **Color temperature**: `/sys/class/disp/disp/attr/color_temperature` — exists on
  H700, same as tg5040. Copy as-is.
- **DisplayCal**: same `/dev/disp` gamma-LUT ioctls (0x10b/0x10c/0x10d) — see 04.
- **Mute**: no `/sys/class/speaker/mute` on H700; use `SPK Switch` mixer ctl.
- **Jack detection**: old note says `snd_soc_sunxi_component_jack/parameters/jack_state`
  never changes; test on 2026 firmware; alternatives: ALSA jack ctl events
  (`amixer contents` may expose a jack bool), or an input event device. `audiomon.elf`
  (tg5040's device-watch daemon) may adapt — investigate its mechanism when wiring.

## keymon (`workspace/h700/keymon/keymon.c`)

Merge of tg5040's structure with old rg35xxplus codes: read `/dev/input/event1`
(ANBERNIC-keys) + `event2` (dierct-keys / volume: PLUS 115, MINUS 114) + `event0`
(power 116). MENU(312)+vol = brightness, SELECT(310)+vol = colortemp (parity with
tg5040), plain vol keys = volume. No mute DIP switch on H700 (drop gpio243 watcher).

## Cores (`workspace/h700/cores/makefile`)

Copy tg5040's verbatim (same aarch64 cortex-a53 target). Do **not** repeat the old
port's "copy from rg35xx" hack. Trim only if something fails to build; expected: the
full 28-core list works.

## What disappears vs tg5040
- `btmanager/` (bluez upgrade pakz — H700 Ubuntu has modern BlueZ)
- `poweroff_next/` (systemd poweroff; verify first)
- LED animation engine wiring (`led_anim`), ledcontrol.elf
- `trimui_inputd` interactions (turbo, gpio107 5V export, etc.)
