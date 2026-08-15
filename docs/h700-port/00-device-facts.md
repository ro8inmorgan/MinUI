# 00 — Device & OS Facts

Ground truth and probe notebook for the H700 platform. Current pass/fail results live
in [08](08-testing-status.md); release decisions live in [09](09-roadmap.md).
Everything here was verified live on hardware
over SSH (probed 2026-07-08, corrections folded in from the implementation/review
cycle through 2026-07-10) or recovered from git history. Re-run the probes after any
Anbernic stock-firmware update — paths have historically been stable, but the
`dmenu_ln`/muOS hooks are stockmod-version-dependent.

## Devices

| Device | Evidence | Role |
|---|---|---|
| TrimUI Brick (TG5040, A133P) | Probed live | Reference — NextUI runs perfectly |
| Anbernic RG40XXV (H700) | Probed live | 640×480, one stick, HDMI; primary probe device |
| Anbernic RG34XXSP (H700) | User-tested | 720×480, dual sticks, clamshell lid sensor |
| Anbernic RG28XX (H700) | User-tested | Rotated 480×640 panel |
| Anbernic RG SP (H700) | Firmware-analysed | 720×480, **no** sticks; RG34XXSP hardware minus the sticks |

### RG SP vs RG34XXSP

Established by comparing the stock `2026-07-27 ANBERNIC RG SP TF1.img` against both
the RG34XXSP stock and StockMod images. The two devices are near-identical: same
DRAM type (boot0 differs by 11 bytes of parameter block), same kernel size to the
byte, and `mali_kbase`/`8821cs`/`rtl_btlpm` with byte-identical `.text`.

The entire device-tree difference is 34 lines:

| Change | Meaning |
|---|---|
| `lcd_*` **unchanged** — `lcd_driver_name = "rg34xxsp_v1"`, 720×480, identical timings | same panel; DTB fingerprint `26-820-536`, the `34xx` entry in the panel-fix table below |
| `pmu_battery_cap` `0xce4` → `0xdac` | 3300 → 3500 mAh |
| `keyL3`/`keyR3` removed; `amux-en-gpios`, `A0_gpio`, `A1_gpio`, `adc-en-gpios` removed | no analog sticks |
| GPADC `status` `okay` → `disabled`, five unused `key0..4_vol/val` added | stick ADC turned off |

⚠ **The stock `bootlogo.bmp` is 640×480 on a 720×480 panel — on the RG34XXSP too.**
It is a stock packaging quirk, not evidence about the panel; StockMod replaces it
with a correct 720×480 image. Do not treat the vendor bootlogo as a geometry source.

## RG40XXV (H700) — probed live

### SoC / kernel / userland
- SoC: Allwinner H700, device-tree compatible: `allwinner,h616`, `arm,sun50iw9p1`
- `/proc/device-tree/model`: `sun50iw9`
- Kernel: `Linux ANBERNIC 4.9.170 #16 SMP PREEMPT aarch64` (built 2026-05-21)
- Userland: **Ubuntu 22.04 (Jammy) arm64, glibc 2.35** — full apt-capable distro,
  systemd, sshd out of the box. **`/bin/sh` is dash, not bash** — pak/launch scripts
  must be POSIX (`&>` silently backgrounds under dash; this broke game launch once).
- RAM: 1 GB (`MemTotal: 996628 kB`)
- CPU: 4× Cortex-A53 (`CPU part 0xd03`), cpufreq 480 MHz – 1.512 GHz, single cluster
  - governors: `interactive conservative ondemand userspace powersave performance schedutil`
  - policy path: `/sys/devices/system/cpu/cpu0/cpufreq/`
- Thermal zones (`/sys/class/thermal/thermal_zone*/type`):
  **zone0=cpu, zone1=gpu, zone2=ve, zone3=ddr, zone4=battery**
  (a first-draft platform.c read zone2 for GPU — wrong; it's the video engine)

### Display
- `/dev/fb0`: 640×480 (virtual_size `640,480`); modes list includes `U:1280x1024p-59`, `U:640x480p-75`, `U:640x480p-59` (HDMI modes appear here)
- Allwinner **disp2** driver: `/dev/disp` char device present
- `/sys/class/disp/disp/attr/` contains: `color_temperature`, `enhance_bright`, `enhance_contrast`, `enhance_saturation`, `enhance_mode`, `xres`, `yres`, … — near-identical attribute set to TG5040 (same disp2 driver family). ⚠ `color_temperature` works; the `enhance_*` attrs accept writes but have **no visible effect** on RG XX panels (unlike tg5040)
- **No `/sys/class/backlight`** — brightness via `/dev/disp` ioctl `DISP_LCD_SET_BRIGHTNESS` (0x102), args `{0, raw 0-255, 0, 0}`. Confirmed working in the shipped port (level curve in 03). There is also a `brightness` file under the AXP battery sysfs used by the stock `brightCtrl.bin` daemon — we kill that daemon at launch.
- Gamma LUT ioctls `DISP_LCD_SET_GAMMA_TABLE (0x10b)` / `GAMMA_CORRECTION_ENABLE (0x10c)` / `DISABLE (0x10d)` — **confirmed working**: displaycal RGB gains visibly act, persist, and survive sleep + game launch on RG40XXV.
- Boot arg: `lcd_type=boe` (panel vendor in cmdline — may vary per unit)
- HDMI hotplug: `/sys/class/extcon/hdmi/` (extcon0); `state` and `cable.0/state` both probed by the shipped `GetHDMI()`.

### GPU — fully verified ✔
- Mali-G31 MP2 ("bifrost"), kernel module `mali_kbase` **r20p0-01rel0 (UK 11.17)**, `/dev/mali0`
- Vendor blob: monolithic `/usr/lib/libmali.so.0.20.0` (19 MB) — **ELF 64-bit aarch64**;
  `libEGL.so.1.4.0` / `libGLESv2.so.2.1.0` / `libGLESv1_CM.so` are 4 KB 64-bit shims over it
- Blob version string: **`OpenGL ES 3.2 v1.r20p0-01rel0`** — userspace r20p0 matches kernel kbase r20p0 ✔
- Blob EGL winsys = **fbdev** (`mali_egl_winsys_fbdev.c` paths in binary) — what the SDL mali-fbdev backend targets. Confirmed end-to-end: NextUI's full GLES 3 shader pipeline (shaders, overlays, effects) runs on it.
- Stock `/usr/lib/libSDL2-2.0.so.0.12.0` is 64-bit, built with only the `mali` + `dummy` video drivers — the stock OS itself uses the SDL-on-mali-fbdev stack.
- **Do not link GLES via the tg5040 SDK's `glesv2` pkg-config** — it drags in libUMP (Utgard-era, absent on Mali-G31 stock). Link `-lGLESv2 -lEGL` directly.
- Mesa copies under `/usr/lib/aarch64-linux-gnu/` are irrelevant on the 4.9 kernel (no DRM).

### Audio
- ALSA cards: `0 audiocodec` (speaker/lineout/headphone), `1 ahubdam`, `2 ahubhdmi` (HDMI audio)
- amixer simple controls on card 0: `'digital volume'`, `'lineout volume'`, `'LINEOUT'`, `'SPK'`, `'OutputL/R Mixer DACL/R'`, `'tx hub mode'`
- `digital volume` is an **attenuator with a reversed scale** (range 0–63; larger raw value = quieter). The shipped msettings writes `100 - val` percent — confirmed correct by listening on device. Volume control has no known issues on RG40XXV or RG34XXSP; the UI and audible levels are correct from mute through 100%.
- Codec supports 48000/44100 natively.
- **Stock libasound quirk (critical):** the device's `/usr/lib/libasound.so` exports both
  versioned symbols and legacy unversioned `ALSA_0.9` value-semantic `snd_pcm_hw_params_set_*`
  symbols. A binary cross-built against a libasound *without* symbol versioning (the tg5040
  SDK's) resolves the unversioned variants at load time → `set_rate_near` behaves like the
  0.9 API and clamped the codec to 192 kHz while SDL believed 32.768 kHz → sliced/glitchy
  audio. Fix: build SDL2 with `--enable-alsa-shared` so it **dlopens** the device libasound
  (dlsym picks the default/new-API symbols). See 05.
- Headphone jack: the codec/hardware auto-mutes the speaker and routes audio without
  NextUI. The old `snd_soc_sunxi_component_jack/parameters/jack_state` software path
  was historically always 0 and remains unused; see 05.

### Input
- `event0` — `axp2202-pek` (power button, PMIC). KEY_POWER = 116.
- `event1` — `ANBERNIC-keys` (gpio-keys-polled): handlers `kbd js0 event1`
  - KEY bits: {1, 114, 115, 304–316, 354} — **no d-pad keycodes**; the d-pad is delivered
    as ABS_HAT0X/Y (the old port's UP 103 / DOWN 108 / LEFT 105 / RIGHT 106 codes do
    **not** appear on current firmware)
  - ABS bits `0x3003c` = ABS_Z, ABS_RX, ABS_RY, ABS_RZ (analog sticks; H700 scales
    the signed values by ×32767/4096) + ABS_HAT0X/Y (d-pad)
  - FF bits present (input-FF rumble is theoretically available; port uses `moto` sysfs instead)
- `event2` — `dierct-keys-polled` (volume keys: PLUS 115, MINUS 114)
- `/dev/input/js0` exists. The in-tree SDL patch restores no-udev classification of
  the built-in pad, but the H700 platform deliberately skips it as an SDL joystick:
  raw evdev already supplies those events, and SDL's built-in ordering does not match
  the external-pad `JOY_*` mapping. SDL joystick input remains enabled for external
  controllers (`SDL_JOYSTICK_DISABLE_UDEV=1`).
- Verified evdev button codes (event1): A 304, B 305, Y 306, X 307, L1 308, R1 309,
  SELECT 310, START 311, MENU 312, L3 313, L2 314, R2 315, R3 316,
  PLUS 115, MINUS 114.
- **MENU emits a compound sequence on short taps** (verified via evtest on RG34XXSP,
  likely all RG XX H700 models): 312 down at press, 312 up at release, then a
  synthetic 354 (`KEY_GOTO`) down at the same instant as the 312 up, 354 up ~190 ms
  later. On a long hold, only 312 fires (down/up, no 354). So 312 tracks the physical
  button; 354 is a firmware "tap detected" pulse and **must not be mapped to
  BTN_MENU** — doing so extends every tap past the 250 ms long-press threshold
  (tap misread as hold → brightness overlay instead of shortcuts). The extra
  KEY_ESC (code 1) capability bit has not been observed to fire.
- After the no-udev classification patch, SDL orders the built-in pad by evdev key
  code: ESC/VOL−/VOL+ occupy indices 0–2 and the gamepad cluster starts at A=3
  through MENU=11. This ordering is diagnostic only; the runtime skips this SDL
  device and reads it through evdev. The `JOY_*` constants in `platform.h` describe
  the expected external-pad mapping, not the built-in pad.
- `evtest` is available on the device for mapping discovery.

### Power / battery / sleep
- `/sys/power/state`: **`freeze mem`** — real suspend-to-RAM (same as TG5040)
- `echo mem` suspend works; wake source is the **power button** (AXP2202 PEK).
  **RTC alarm wake does NOT fire** — no timed wake; don't build features on it.
  The shipped sleep/wake path is reliable in repeated RG40XXV/RG34XXSP testing,
  including in-game resume and power-off auto-resume. On RG34XXSP, POWER can wake
  light sleep while the lid is closed; deep sleep re-sleeps if the lid remains closed
  (06). Charging intentionally prevents deep sleep in shared code.
- PMIC: **AXP2202** — `/sys/class/power_supply/axp2202-battery/` and `axp2202-usb/`
- Battery: `capacity`, `status`, `voltage_now`, `temp`, `time_to_empty_now`,
  `time_to_full_now`, `charge_counter`, `health`
- Anbernic kernel extras under `axp2202-battery/`:
  - `moto` — rumble motor (write 1/0) — **confirmed working**
  - `work_led` — small power-indicator LED, next to the charge LED. Present on every
    H700 model, unrelated to the RGB stick LEDs. **Polarity is `0=off, 1=on`** — an
    earlier note here had it backwards. Verified the same way on RG40XXV and RG28XX:
    the LED is lit through stock boot and goes out as soon as NextUI's `launch.sh`
    writes `0`. Plugging in USB lights it independently of any of this.
    The port writes `0` on wake and `1` on sleep (`PLAT_enableBacklight`), so in
    practice the LED marks *sleep*, not power. That is inherited from the old
    rg35xxplus port, is the same across models, and is deliberate — don't "correct"
    the polarity of those writes on the strength of the old comment.
  - `workled_sleep` — LED behavior during sleep
  - `lowpwr_led`, `led_test`
  - `mcu_pwr` — power rail for the RGB LED MCU (write 1 to enable). See "RGB LEDs" below.
  - `hallkey` — lid/hall sensor (RG34XXSP only; absent on RG40XXV) — polarity verified
    (`1` = open); lid close sleeps and lid open wakes screen-off. Deep suspend still
    requires power. POWER can wake light sleep while closed; deep sleep re-sleeps if
    the lid remains closed (06).
  - `brightness`, `display_id` (panel variant id), `spk_state`, `mcu_esckey`, `nds_esckey`, `boot_mode`
- `/sys/class/pwm/pwmchip0` exists (alternative rumble path; `moto` is simpler)

### RGB LEDs

Only three models have them: **RG40XX H, RG40XX V, RG CubeXX** (the same three muOS
flags with `device/<dev>/config/led/rgb = 1`; every other RG XX is `0`).

An early probe concluded RG XX had no RGB LEDs. That was wrong. They are not exposed
as LED-class devices — on an RG40XXV `/sys/class/leds` is empty, there is no
`led_anim` driver (TrimUI only), and the device tree has no LED node. They hang off a
**separate MCU reached over UART5**:

- `/dev/ttyS5`, 115200 8N1 raw (`uart@05001400`; char 248,5 — verified present)
- `/sys/class/power_supply/axp2202-battery/mcu_pwr` must be `1` to power the MCU
- Frame: `<mode> <brightness> <payload...> <checksum>`, checksum = `sum(preceding) & 0xFF`,
  written as raw bytes. Brightness is one byte for the whole strip (0-255).
  - mode 1 solid — payload 8x(R,G,B) for one bank then 8x(R,G,B) for the other
    (16 positions in two banks of 8); the only mode with per-bank colour
  - mode 2/3/4 breath fast/med/slow — payload 16x(R,G,B), one colour for all
  - mode 5/6 rainbow mono/multi — payload `<1> <1> <speed 0-255>`
- The MCU animates on its own, so there is no userspace animation in the port —
  effect ids are translated onto these modes in `workspace/h700/platform/led.c`.
- Protocol source: muOS `MustardOS/internal`, `script/device/rgb.sh` (SERIAL backend).

### Network / Bluetooth
- WiFi: RTL8821CS (SDIO), module `8821cs`; interfaces `wlan0` (+`wlan1` virtual);
  `/sys/class/net/wlan0/operstate` for link state
- Stock runs wpa_supplicant (`-u -s` dbus mode) + NetworkManager + dnsmasq — the port
  stops NetworkManager and owns its own wpa_supplicant instance (07)
- **WiFi does not recover on its own after `mem` suspend** — confirmed; the suspend
  script bounces it on resume (06/07)
- Bluetooth: `rtl_btlpm` module and stock BlueZ (5.66 on clean 2026 RG40XXV;
  5.64 was observed on earlier firmware). The stock frontend normally runs
  `rtk_hciattach -n -s 115200 ttyS1 rtk_h5`, but NextUI replaces that frontend
  before it performs the attach, so the H700 init script must invoke the vendor
  `setBluetooth.sh` path itself.
- The clean 2026 RG40XXV image contains BlueALSA 4.2.0, its ALSA PCM/control
  plugins, ALSA configuration, and D-Bus policy, contrary to the earlier probe
  recorded here. Its `a2dp-source` profile successfully registers SBC media
  endpoints with stock BlueZ. NextUI uses these stock components directly and does
  not bundle or replace BlueALSA, its ALSA plugins, SBC, or BlueZ. Ubuntu has no
  `udhcpc` (DHCP = `dhclient`).

### Storage & partitions (TF1 = the stock OS card)
```
mmcblk0p1..4  special/boot-resource/env/boot (u-boot, kernel, DTB…)
mmcblk0p5  →  /            ext4  (Ubuntu rootfs)
mmcblk0p6  →  /mnt/vendor  ext4  (stock frontend, emulators, libs)
mmcblk0p7  →  /mnt/data    ext4  (also used as swap by loadapp.sh)
mmcblk0p8  →  /mnt/mmc     vfat  (ROMs partition — user-visible FAT32; our dmenu.bin lives here)
```
- TF2 (second slot): `/dev/mmcblk1p1` → stock mountpoint `/mnt/sdcard`; the port
  mounts it itself when needed (vfat or exfat via helper, with fsck repair — see 02)
- DTB is read raw from `/dev/mmcblk0` by the old "Panel Fix" tool (offset 17954816, magic `d00dfeed`)
- Kernel 4.9 has no native exfat; boot shim falls back through vfat → exfat → auto

### Stock OS boot chain (THE hijack point)
systemd `launcher.service` → `/etc/init.d/launcher.sh start` → starts `brightCtrl.bin` +
`cexpert` + `/mnt/vendor/ctrl/loadapp.sh` → (swap, resize, retroarch cfg) →
`/mnt/vendor/ctrl/dmenu_ln`, which does:

```sh
CMD="/mnt/vendor/bin/dmenu.bin"          # stock fallback
MISCBIN="/mnt/mmc/dmenu.bin"             # ← OUR HIJACK POINT (FAT partition!)
if [ -f "/mnt/vendor/muos1.ini" ]; then MISCBIN="/mnt/vendor/bin/muos1.bin"; fi   # stockmod!
if [ -f "/mnt/vendor/muos2.ini" ]; then MISCBIN="/mnt/vendor/bin/muos2.bin"; fi
[ -f $MISCBIN ] && CMD=$MISCBIN
...
if $CMD; then
    [ -f /tmp/.next ] && sh /tmp/.next    # chain-exec mechanism after CMD exits
fi
```

Consequences:
1. Dropping an executable named **`dmenu.bin` onto the FAT ROMs partition (`/mnt/mmc`)**
   makes the stock OS run it instead of its own frontend. No reflashing. This is how the
   shipped port (and old MinUI, and muOS "in-place") installs.
2. **MU-style theme caveat:** selecting stock/stockmod **MU style 1/2** drops
   `muos1.ini`/`muos2.ini` under `/mnt/vendor`, which *override* the
   `/mnt/mmc/dmenu.bin` check so the MU frontend runs instead. NextUI never starts
   and cannot show a warning from that path. Install requires stock **old style**
   theme (documented in `skeleton/BASE/README.txt`; see 02).
3. `/tmp/.next` chain-exec exists in the stock wrapper; NextUI runs its own launch loop.
4. `launcher.sh stop` kills via `SIGUSR1 dmenu.bin` — the shim `trap`s USR1.
5. Model detection: `strings /mnt/vendor/bin/dmenu.bin | grep -m1 ^RG` → `RGXX_MODEL`
   env — **confirmed working on 2026 firmware** (RG40XXV). Fallback detectors if it ever
   breaks: fb0 mode `480x640` → 28xx, `xres/yres` in /sys/class/disp,
   `axp2202-battery/display_id`, DTB lcd timings.

   Strings read straight out of the vendor `dmenu.bin` in each firmware's `appfs`
   (p6, the partition mounted at `/mnt/vendor`): **`RGSP`**, `RG34xx`, `RG40xxH`.
   ⚠ The RG SP's is a bare **`RGSP`** — no `xx` — so it matches none of the
   `RG34xx*`/`RG40xx*` family globs in launch.sh and needs its own case. Before that
   case existed it hit the `*) DEVICE="rg40xx"` fallback and NextUI drew the UI at
   640×480 on a 720×480 panel, which is the "4:3 instead of 3:2" symptom users saw.

## TG5040 (reference) — probed live
- Kernel `4.9.191 aarch64` (TinaLinux/OpenWRT-flavored), **glibc 2.33**
- `/sys/power/state`: `freeze mem`; same disp2 attrs incl. `color_temperature`;
  also no `/sys/class/backlight`
- NextUI install: `/mnt/SDCARD/.system/tg5040/{bin,lib,cores,paks,shaders,etc}`.
  **SDL2 comes from the stock OS there** — on H700 we bundle our own (04).

## glibc compatibility conclusion

| | kernel | arch | glibc |
|---|---|---|---|
| TG5040 | 4.9.191 | aarch64 | 2.33 |
| H700 stock | 4.9.170 | aarch64 | **2.35** |

h700-toolchain-built binaries (GCC 8.3, glibc ≤ 2.33 from the TG5040 SDK sysroot,
aarch64, cortex-a53) load and run on the H700 stock OS unmodified — proven in
production by this port (first via the shared tg5040 image, now via dedicated
`h700-toolchain`). No ABI wall. (Caveat discovered the hard way: libc compatibility
is not the whole story — see the libasound symbol-versioning and libpng12 pitfalls in 01.)

## Old rg35xxplus port — git archaeology summary

- Last full commit at its canonical path: **`8cd78866`**
  (`git show 8cd78866:workspace/rg35xxplus/<path>`). Renamed to `workspace/_unmaintained/`
  in `fc9a5f3a`, deleted in `57e53bbe`.
- Was **32-bit** (`arm-buildroot-linux-gnueabihf`), toolchain `LoveRetro/rg35xxplus-toolchain`;
  custom SDL2 2.28.5 from `JohnnyonFlame/SDL-malifbdev-rot` (the same repo the new port uses, rebuilt 64-bit)
- Its sysfs inventory was the hardware-paths reference for this port; everything relevant
  is re-verified and captured above. Remaining old-port material of interest:
  - Panel geometry per model: default (RG35XX+/H/SP, RG40XX*) 640×480; RG34xx 720×480;
    RGcubexx 720×720 (only overscan-capable device); RG28XX physically 480×640 portrait
  - Panel-fix DTB timing table (SKU fingerprints, `lcd_dclk_freq-lcd_ht-lcd_vt`):
    `24-770-526`=35xx2024/Plus, `24-770-528`=35xxH, `24-586-686`=28xx, `24-770-525`=35xxSP,
    `24-770-522`=40xxH, `36-812-756`=CubeXX, `26-820-536`=34xx
  - HDMI machinery: `hdmimon.sh` via `/sys/kernel/debug/dispdbg` + `fbset`
    (`git show 8cd78866:skeleton/SYSTEM/rg35xxplus/bin/hdmimon.sh`)
- Scope note: the old port is a hardware-paths reference only; its software architecture
  (SDL_Renderer compositing, 32-bit, pre-shader APIs) is obsolete. tg5040 was the code
  clone base.

## Known hardware unknowns

1. Real panel refresh rate — `SCREEN_FPS 60.0` is assumed, never measured.
2. Exact stock `RGXX_MODEL` strings for the RG35XX family. (RG40XXH is now
   confirmed as `RG40xxH`, read from its StockMod `dmenu.bin`.)
3. RG SP support is derived entirely from firmware analysis — never run on the
   hardware. Panel geometry, stickless input, and the calibrated displaycal
   preset all need confirming on a real unit.
