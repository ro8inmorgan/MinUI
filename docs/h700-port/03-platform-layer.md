# 03 — Platform Layer (`workspace/h700/`)

Strategy, as planned and executed: **clone `workspace/tg5040/`**, keep the
`generic_video.c`/`generic_wifi.c`/`generic_bt.c` includes and the current `PLAT_*` API
surface, repoint hardware access to the H700 paths (00), borrowing only hardware
constants from the old rg35xxplus port (`git show 8cd78866:...`).

One platform serves all devices: `DEVICE`/`RGXX_MODEL` env (set by launch.sh, see 02)
→ `detect_device()` sets `is_rg28xx / is_rg34xx / is_cube` globals that drive
resolution and the bootlogo preview rotation (panel rotation itself is handled
entirely by the SDL driver via `SDL_ROTATION=1`; nothing app-side rotates — 04),
plus `dev_has_lstick / dev_has_rstick` capability flags
that drive stick availability — the tg5040 `is_brick` pattern.

Analog stick matrix (verified against Retro Catalog/Anbernic specs 2026-07, RG40XXV
and RG34XXSP confirmed on hardware; every stick present clicks, so L3 ⇔ left stick,
R3 ⇔ right):

| Device | Sticks | Detection key |
|---|---|---|
| RG28XX, RG34XX, RG35XX Plus/2024/SP | none | `RG28xx` / `RG34xx` exact; `RG35xx` prefix default |
| RG35XX H, RG35XX Pro | dual | `RG35xx` prefix + `H`/`Pro` suffix |
| RG34XXSP | dual | `RG34xxSP` exact |
| RG40XX H, RG CubeXX, unknown | dual | `RG40xxH`, `RGcube*`; dual is the fallback |
| RG40XX V | left only | `RG40xxV` exact |

Unknown `RG35xx` variants default to stickless (most of that family is); anything
else unknown defaults to dual (today's behavior, covers `DEVICE=rg40xx` without a
model string). No H700 Anbernic device has an Fn switch. Exact `RGXX_MODEL` strings
still unconfirmed for the RG35xx family and RG40xxH (same caveat as msettings'
displaycal presets).

The Input pak intentionally visualizes digital button/click state only. It detects
stick availability and draws L3/R3, but `minput.c` has no rendering path for analog
axis movement; RG34XXSP showing working L3/R3 without moving-stick graphics is expected
and is not an H700 regression.

## platform.h (as shipped)

```c
extern int is_rg28xx, is_rg34xx, is_cube;
extern int dev_has_lstick, dev_has_rstick;

#define FIXED_SCALE   2
#define FIXED_WIDTH   (is_cube?720:(is_rg34xx?720:640))
#define FIXED_HEIGHT  (is_cube?720:480)
#define FIXED_BPP     2
#define SCREEN_FPS    60.0            // ⚠ assumed, never measured per panel
#define SDCARD_PATH   "/mnt/SDCARD"   // symlink to the real TF2 mountpoint (02)
#define MAX_LIGHTS    0               // no RGB LEDs on RG XX (work_led is on/off only)
#define MAIN_ROW_COUNT (is_cube?8:6)  // 480px-tall screens fit 6 rows
```

## Input — dual path, evdev primary (deviation from plan)

The plan preferred the tg5040-style pure-SDL-joystick route. **Shipped: raw evdev is
the primary path for built-in controls**, with SDL joystick as a secondary path for
Bluetooth controllers. Reasons discovered during bring-up:
- SDL joystick enumeration of the built-in pad is unreliable on the stock image
  (udev interplay); SDL is built without udev and `SDL_JOYSTICK_DISABLE_UDEV=1` is
  exported — that fixed startup hangs but made js enumeration of gpio-keys flaky.
  Root cause found later (2026-07-09, Files-app freeze): the SDL fork's Batocera
  patches deleted the joystick heuristic in `SDL_EVDEV_GuessDeviceClass()`, so the
  no-udev fallback path never classified any device as a joystick. Fixed by
  `workspace/h700/patches/sdl2-h700.patch` (see 01); SDL joystick enumeration of
  the built-in pad now works. Note the fork assigns SDL button indices in
  ascending evdev-keycode order, so the pad's ESC/VOL−/VOL+ (1/114/115) occupy
  indices 0–2 and the gamepad cluster starts at index 3 (A=3 … MENU=11).
  **Because of that, platform.c must never open the built-in pad as an SDL
  joystick**: `poll_sdl_input()` interprets SDL joysticks with the BT-pad
  `JOY_*` layout, so the built-in pad's events would double-apply with scrambled
  meanings (B→L1, VOL−→back, MENU→volume UI — seen in Settings 2026-07-09).
  `is_builtin_pad()` skips "ANBERNIC-keys" in both the init scan and the
  hotplug path; evdev is the sole path for built-in controls, SDL joystick is
  for external (BT) pads only.
- evdev codes are needed for keymon and wake handling anyway.

Implementation: `poll_evdev_input()` scans/reads `/dev/input/event0..11` directly with
periodic (2 s) rescans for hotplug, mapping via `CODE_*` defines; `poll_sdl_input()`
handles SDL joystick events (BT pads) via `JOY_*` indices.

Verified mappings (also recorded in 00):
- evdev (event1 `ANBERNIC-keys`): A 304, B 305, Y 306, X 307, L1 308, R1 309,
  SELECT 310, START 311, MENU 312, L3 313, L2 314, R2 315, R3 316; PLUS 115 /
  MINUS 114 (event2); POWER 116 (event0, `axp2202-pek` — note tg5040 uses 102).
  **D-pad has no keycodes on current firmware — it arrives as ABS_HAT0X/Y.**
- **MENU compound-tap quirk (fixed)**: the firmware reports the physical MENU button
  faithfully on 312 (down while held, up on release), but on a *short tap* it also
  emits a synthetic 354 (`KEY_GOTO`) pulse that starts the instant 312 releases and
  lasts ~190 ms (verified via evtest on RG34XXSP; on a long hold 354 never fires).
  Mapping both 312 and 354 to `BTN_MENU` (as the old rg35xxplus platform did via
  `CODE_MENU_ALT`) stretched every tap past the 250 ms `MENU_DELAY` threshold —
  a quick MENU tap registered as a hold, so the home screen flipped from the
  shortcuts overlay into brightness mode. Fix: `button_from_code()` no longer maps
  `CODE_MENU_ALT` to `BTN_MENU` (keymon likewise ignores it); tap-vs-hold is derived
  from the clean 312 timing. The SDL path was never affected (`JOY_MENU_ALT = JOY_NA`).
- Analog sticks: ABS_Z/RX/RY/RZ, raw 0..4096, scaled ×32767/4096. Presence varies
  per model (see the stick matrix above); `dev_has_lstick`/`dev_has_rstick` gate
  CODE_L3/R3, JOY_L3/R3, and AXIS_LX/LY/RX/RY to NA where the stick is absent, so
  the Input pak and shared input code adapt without platform-independent changes.
  (Previously CODE_L3/R3 were unconditional, so the Input pak drew L3/R3 pills on
  every device, and `is_rg34xx` wrongly stripped L3/R3 from the RG34XXSP.)
- SDL indices (secondary path): A=0 B=1 Y=2 X=3 L1=4 R1=5 SELECT=6 START=7 MENU=8,
  L3=9 L2=10 R2=11 R3=12, MINUS=15 PLUS=16; axes LX=0 LY=1 RX=2 RY=3.

Button semantics: `BTN_RESUME=BTN_X`, `BTN_SLEEP/WAKE=BTN_POWER`,
`BTN_MOD_BRIGHTNESS=BTN_MENU`, `BTN_MOD_COLORTEMP=BTN_SELECT`, dedicated
`BTN_MOD_PLUS/MINUS` on the volume keys — parity with tg5040 conventions.

`PLAT_shouldWake` keeps a **persistent** `wake_fd` on event0 (opened once
O_NONBLOCK|O_CLOEXEC, drained per poll, closed in `PLAT_quitInput`) — an early version
open/closed per poll and could drop the wake press between polls.

## platform.c — subsystem map (as shipped)

| Subsystem | Implementation |
|---|---|
| Video | `#include "generic_video.c"` — custom SDL2 mali driver does the rest (04) |
| Battery | `axp2202-battery/capacity` + `axp2202-usb/online`, coarse bucketing in shared code — identical to tg5040 |
| CPU speed | `governor.sh` via `system()`: auto=schedutil, performance=max 1512000, powersave=conservative capped mid-range. Manual changes work. RG34XXSP MD testing exposed render-setting-sensitive auto frequencies (often 480 MHz when slow; ~720 MHz when smooth), still under investigation. Single A53 cluster → `PLAT_pinToCores` no-op |
| CPU temp | thermal_zone0 |
| GPU temp | thermal_zone1 (zone map in 00 — zone2 is the video engine, a first draft got this wrong) |
| GPU speed | devfreq `cur_freq` (two SoC paths) → debug clk paths → 660 MHz literal as last-resort fallback |
| Rumble | `echo 1/0 > axp2202-battery/moto` — on/off only, strength>0 → 1. Works (tested). Input-FF (event1 advertises FF bits) unexplored |
| LEDs | `MAX_LIGHTS 0`, all `PLAT_setLed*` stubs — hardware has no RGB LEDs. `work_led` used only as sleep/backlight indicator |
| Backlight | raw brightness 0 via disp ioctl + fb blank + `work_led` on/off around it |
| Lid | `hallkey` path wired into `PLAT_initLid`/`PLAT_lidChanged` (`has_lid` = file exists); lid close → sleep and lid open wakes screen-off. RG34XXSP issue: power also wakes light sleep while the lid is closed despite the intended gate. |
| Model | `PLAT_getModel` → "Anbernic " + `RGXX_MODEL` (copied into a static buffer, not a raw getenv pointer) |
| Date/time | `timedatectl` / `hwclock` / `date` via snprintf-bounded commands; timezones via `timedatectl set-timezone`/`list-timezones`, NTP via `set-ntp` (systemd-timesyncd) — much cleaner than tg5040's uci |
| Turbo | `PLAT_canTurbo()=false`, no-ops (tg5040's turbo rides trimui_inputd; no H700 equivalent wired) |
| Deep sleep | `PLAT_supportsDeepSleep()=1`; `suspend` script wraps `echo mem` (06) |
| Sample rate | tg5040 logic; consults `PLAT_bluetoothConnected()` only — calling `GetAudioSink()` here segfaulted minarch pre-`InitSettings()` (shm not yet mapped) |
| WiFi/BT | generic_wifi.c / generic_bt.c + init scripts (07) |

## libmsettings (`workspace/h700/libmsettings/msettings.c`, ~1300 lines)

- **Brightness**: `/dev/disp` ioctl `DISP_LCD_SET_BRIGHTNESS (0x102)`, args
  `{0, raw, 0, 0}`; level curve `0→4, 1→6, 2→10, 3→16, 4→32, 5→48, 6→64, 7→96,
  8→128, 9→192, 10→255`. Stock `brightCtrl.bin` is killed at launch so it can't fight us.
- **Volume**: tinyalsa on card 0. `digital volume` is a 0–63 **attenuator** — the code
  writes `mixer_ctl_set_percent(digital, 0, 100 - val)` (reversed mapping, confirmed
  correct by ear; the control's TLV metadata is garbage). `lineout volume` secondary.
  Tested on RG40XXV and RG34XXSP: UI and audible levels are correct from mute through
  100%, with no current volume-control issues.
- **Mute**: `SPK` switch off + store/restore volume (no `/sys/class/speaker/mute` on
  H700). The physical volume UI still supports mute; Fn-switch settings are gated off
  because RG XX devices have no Fn switch.
- **Color temperature**: `/sys/class/disp/disp/attr/color_temperature` — works
  (tested ✅). The sibling `enhance_contrast` / `enhance_saturation` /
  `enhance_bright` attrs are also written, **but have no visible effect on RG XX
  panels** — those settings are gated off for h700 (09-roadmap #8). All
  `scale*()` switches carry `default:` cases (a draft could hit uninitialized
  values on out-of-range input).
- **DisplayCal**: same gamma-LUT ioctls as tg5040 (0x10b/0x10c/0x10d) — worked 1:1 as
  predicted; tested passing on RG40XXV including persistence across sleep and game
  launch.
- **HDMI**: `GetHDMI()` probes `/sys/class/extcon/hdmi/{state,cable.0/state,...}` and
  `SetHDMI()` switches disp0, framebuffer/layer geometry, and the ALSA route. Menu,
  game, audio, and in-game hotplug in both directions pass on RG40XXV (04).
- **Jack detection**: not wired (path exists but historically dead — 00).
- Debug printfs were stripped/commented for release; remaining prints are error paths.

## keymon (`workspace/h700/keymon/keymon.c`)

tg5040 structure + H700 codes: reads event1 (buttons) + event2 (volume 115/114) +
event0 (power 116). MENU+vol = brightness, SELECT+vol = colortemp, plain vol = volume —
tg5040 parity. No mute DIP switch on H700 (gpio243 watcher dropped). Tracks MENU on
code 312 only — the synthetic 354 post-tap pulse is ignored (see MENU quirk above).

## Cores (`workspace/h700/cores/`)

tg5040's list verbatim — the full 28-core set builds and ships (same aarch64/a53
target), with the picodrive LTO/patch-hygiene fixes noted in 01.

## What disappeared vs tg5040 (as planned)
- `btmanager/` (BlueZ-upgrade pakz — H700 Ubuntu has BlueZ 5.64)
- `poweroff_next/`, `reboot_next` (systemd poweroff/reboot work fine)
- LED animation wiring (`led_anim`), ledcontrol.elf (gated to tg50x0 in `workspace/makefile`)
  — bootlogo was initially gated off too, but has since been ported: `Bootlogo.pak`
  builds for h700 with resolution-keyed preset folders (`640x480`, `720x480`,
  `480x640`, `720x720`) selected via `$DEVICE`, writes `bootlogo.bmp` to `mmcblk0p2`
  (`BOOTLOGO_PARTITION` in `platform.h`), backs up the stock logo as
  `original.bmp` on first apply (see 02 for the TF1-write-policy exception), and on
  the RG28XX rotates previews to boot orientation (`BOOTLOGO_PREVIEW_ROTATE_CW`, 04)
- `trimui_inputd` interactions (turbo, gpio 5V export)

New vs tg5040: `rfkill/` — a minimal from-scratch `/dev/rfkill` ioctl tool (~120
lines), since the stock binary can't be relied on.
