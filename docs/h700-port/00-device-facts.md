# 00 — Device & OS Facts (verified 2026-07-08)

Everything in this document was verified live on the actual hardware over SSH, or
recovered from git history. Treat it as the ground truth the rest of the plan builds on.

## Devices used

| Device | IP | Login | Role |
|---|---|---|---|
| TrimUI Brick (TG5040, A133P) | 192.168.34.81 | root / tina | Reference — NextUI runs perfectly |
| Anbernic RG40XXV (H700) | 192.168.34.55 | root / root | Port target #1 (running stockmod) |
| Anbernic RG34XXSP (H700) | — | — | Port target #2 (clamshell, lid sensor) |
| Anbernic RG28XX (H700) | — | — | Port target #3 (rotated 480×640 panel, phase 2) |

## RG40XXV (H700) — probed live

### SoC / kernel / userland
- SoC: Allwinner H700, device-tree compatible: `allwinner,h616`, `arm,sun50iw9p1`
- `/proc/device-tree/model`: `sun50iw9`
- Kernel: `Linux ANBERNIC 4.9.170 #16 SMP PREEMPT aarch64` (built 2026-05-21 — latest stock)
- Userland: **Ubuntu 22.04 (Jammy) arm64, glibc 2.35** — full apt-capable distro
- RAM: 1 GB (`MemTotal: 996628 kB`)
- CPU: 4× Cortex-A53 (`CPU part 0xd03`), cpufreq 480 MHz – 1.512 GHz
  - governors: `interactive conservative ondemand userspace powersave performance schedutil`
  - policy path: `/sys/devices/system/cpu/cpu0/cpufreq/` (single policy0-style cluster)

### Display
- `/dev/fb0`: 640×480 (virtual_size `640,480`); modes list includes `U:1280x1024p-59`, `U:640x480p-75`, `U:640x480p-59` (HDMI modes appear here)
- Allwinner **disp2** driver: `/dev/disp` char device present
- `/sys/class/disp/disp/attr/` contains: `color_temperature`, `enhance_bright`, `enhance_contrast`, `enhance_saturation`, `enhance_mode`, `xres`, `yres`, … — **near-identical attribute set to TG5040** (same disp2 driver family)
- `color_temperature` currently `0`
- **No `/sys/class/backlight`** — brightness is done via `/dev/disp` ioctl `DISP_LCD_SET_BRIGHTNESS` (0x102), same as TG5040/old-rg35xxplus. There is also a `brightness` file under the AXP battery sysfs (currently `3`) used by the stock `brightCtrl.bin` daemon.
- Boot arg: `lcd_type=boe` (panel vendor in cmdline — may vary per unit)
- HDMI hotplug: `/sys/class/extcon/hdmi/` (extcon0). Old MinUI keymon used `/sys/class/extcon/hdmi/cable.0/state`, platform.c used `/sys/class/switch/hdmi/cable.0/state` — verify which exists on current firmware (extcon confirmed present).

### GPU — fully verified ✔
- Mali-G31 MP2 ("bifrost"), kernel module `mali_kbase` **r20p0-01rel0 (UK 11.17)**, `/dev/mali0` present
- Vendor blob: monolithic `/usr/lib/libmali.so.0.20.0` (19 MB) — **ELF 64-bit aarch64**;
  `libEGL.so.1.4.0` / `libGLESv2.so.2.1.0` / `libGLESv1_CM.so` are 4 KB 64-bit shims over it
- Blob version string: **`OpenGL ES 3.2 v1.r20p0-01rel0`** — userspace r20p0 matches kernel kbase r20p0 ✔
- Blob EGL winsys = **fbdev** (`mali_egl_winsys_fbdev.c` paths in binary) — exactly what the SDL mali-fbdev backend targets
- Stock `/usr/lib/libSDL2-2.0.so.0.12.0` is **64-bit and built with the `mali` video driver** (only `mali` + `dummy` compiled in) — the stock OS itself uses the SDL-on-mali-fbdev stack we plan to use; usable as a bring-up crutch before our own SDL 2.28+ build lands
- Mesa copies also exist under `/usr/lib/aarch64-linux-gnu/` (`libEGL_mesa.so`) — irrelevant on 4.9 kernel (no DRM), the blob is what works

### Audio
- ALSA cards: `0 audiocodec` (speaker/lineout), `1 ahubdam`, `2 ahubhdmi` (HDMI audio)
- amixer simple controls on card 0: `'digital volume'`, `'lineout volume'`, `'LINEOUT'`, `'SPK'`, `'OutputL/R Mixer DACL/R'`, `'tx hub mode'`
- Old MinUI used `amixer sset 'lineout volume' <pct>%` — control still exists ✔

### Input
- `event0` — `axp2202-pek` (power button, PMIC)
- `event1` — `ANBERNIC-keys` (gpio-keys-polled): handlers `kbd js0 event1`; has KEY bits, ABS bits `0x3003c` (= ABS_Z, ABS_RX, ABS_RY, ABS_RZ + ABS_HAT0X/Y) and **FF bits (rumble via input FF!)**
- `event2` — `dierct-keys-polled` (volume keys, function keys)
- `/dev/input/js0` exists — the kernel already exposes the pad as a joystick
- `evtest` is available on the device for mapping discovery

### Power / battery / sleep
- `/sys/power/state`: **`freeze mem`** — real suspend-to-RAM supported (same as TG5040)
- **Verified live: `echo mem` suspend works** (device suspended via `rtcwake -m mem`); RTC alarm wake did **not** fire (device stayed asleep) → power button is the wake source; treat RTC wake as unavailable
- PMIC: **AXP2202** — `/sys/class/power_supply/axp2202-battery/` and `axp2202-usb/`
- Battery: `capacity` (58 at probe time), `status` (`Charging`), `voltage_now`, `temp`, `time_to_empty_now`, `time_to_full_now`, `charge_counter`, `health`
- Anbernic's kernel exposes extra controls under `axp2202-battery/`:
  - `moto` — rumble motor (old MinUI RUMBLE_PATH; write 1/0)
  - `work_led` — power LED (0=on, 1=off)
  - `workled_sleep` — LED behavior during sleep
  - `lowpwr_led`, `led_test`
  - `hallkey` — **lid/hall sensor (RG34XXSP!)** (old MinUI LID_PATH; not present on non-clamshell units — probe per device)
  - `brightness`, `display_id` (panel variant id), `spk_state`, `mcu_esckey`, `nds_esckey`, `boot_mode`
- `/sys/class/pwm/pwmchip0` exists (alternative rumble path; `moto` is simpler)
- `rtcwake` binary exists; RTC wake unreliable (see above)

### Network / Bluetooth
- WiFi: RTL8821CS (SDIO), module `8821cs`; interfaces `wlan0`, `wlan1`
- Stock runs **wpa_supplicant** (`-u -s -O /run/wpa_supplicant`) + **NetworkManager** + dnsmasq
- Bluetooth: `rtl_btlpm` module, `bluetoothd` running, `rtk_hciattach -n -s 115200 ttyS1 rtk_h5` (UART-attached Realtek BT)

### Storage & partitions (single SD "TF1" holds the whole stock OS)
```
mmcblk0p1..4  special/boot-resource/env/boot (u-boot, kernel, DTB…)
mmcblk0p5  →  /            ext4  (Ubuntu rootfs)
mmcblk0p6  →  /mnt/vendor  ext4  (stock frontend, emulators, libs)
mmcblk0p7  →  /mnt/data    ext4  (also used as swap by loadapp.sh)
mmcblk0p8  →  /mnt/mmc     vfat  (ROMs partition — user-visible FAT32)
```
- TF2 (second SD slot) mounts at **`/mnt/sdcard`** (empty at probe — no card inserted); `/mnt/udisk` also exists
- DTB is read raw from `/dev/mmcblk0` by the old "Panel Fix" tool (offset 17954816, magic `d00dfeed`)

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
1. Dropping an executable named **`dmenu.bin` onto the FAT ROMs partition (`/mnt/mmc`)** makes the stock OS run it instead of its own frontend. No reflashing needed. This is exactly how old MinUI (and muOS "in-place") installed.
2. **stockmod caveat:** this RG40XXV has `/mnt/vendor/muos1.ini`, which *overrides* the `/mnt/mmc/dmenu.bin` check. The installer/docs must have the user boot into "stock" mode (or delete `muos1.ini`) for the hijack to engage.
3. `/tmp/.next` chain-exec exists in the stock wrapper, but NextUI runs its own launch loop anyway once dmenu.bin is ours.
4. `launcher.sh stop` kills via `SIGUSR1 dmenu.bin` — our binary should tolerate that.
5. LED config during app switch: stockmod writes to `axp2202-battery/work_led` from `led_*.cfg` files — harmless.

## TG5040 (reference) — probed live
- Kernel `4.9.191 aarch64` (TinaLinux/OpenWRT-flavored), **glibc 2.33**
- `/sys/power/state`: `freeze mem` (same suspend mechanism)
- `/sys/class/disp/disp/attr/` — same attribute set as H700 incl. `color_temperature`
- Also **no `/sys/class/backlight`** (brightness via `/dev/disp` ioctl)
- NextUI install: `/mnt/SDCARD/.system/tg5040/{bin,lib,cores,paks,shaders,etc}` — bundles `libmsettings.so`, `libsamplerate`, `libzip`, etc. **SDL2 comes from the stock OS** (`/usr/trimui/lib`), *not* bundled — on H700 we must bundle our own SDL2 (see 04).
- Running processes: `nextui.elf`, `keymon.elf`

## glibc compatibility conclusion

| | kernel | arch | glibc |
|---|---|---|---|
| TG5040 | 4.9.191 | aarch64 | 2.33 |
| H700 stock | 4.9.170 | aarch64 | **2.35** |

Anything built by the existing tg5040 toolchain (targets glibc ≤ 2.33, aarch64,
cortex-a53) **loads and runs on the H700 stock OS unmodified** as far as libc is
concerned. The port can even bootstrap by copying tg5040-built binaries over for
early experiments. The H700 gets its own toolchain for cleanliness (see 01), but
there is no ABI wall between the two platforms. This is the single biggest
difference vs the old 32-bit rg35xxplus port (which targeted `arm-buildroot-linux-gnueabihf`).

## Old rg35xxplus port — git archaeology summary

- Last full commit of the platform at its canonical path: **`8cd78866`**
  (`git show 8cd78866:workspace/rg35xxplus/<path>`). Renamed to `workspace/_unmaintained/` in `fc9a5f3a`, deleted in `57e53bbe`.
- Was **32-bit** (`arm-buildroot-linux-gnueabihf`, `-marm -mfpu=neon-fp-armv8`), toolchain `LoveRetro/rg35xxplus-toolchain`
- Custom SDL2 2.28.5 from **`JohnnyonFlame/SDL-malifbdev-rot`** (`--enable-video-mali`, everything else disabled) — Mali fbdev backend with rotation support
- Its verified /sys inventory (all confirmed still present on 2026 stock firmware, except where noted):

| Subsystem | Path |
|---|---|
| Battery % | `/sys/class/power_supply/axp2202-battery/capacity` ✔ confirmed |
| Charging | `/sys/class/power_supply/axp2202-usb/online` |
| LED | `/sys/class/power_supply/axp2202-battery/work_led` ✔ confirmed |
| Rumble | `/sys/class/power_supply/axp2202-battery/moto` ✔ confirmed |
| Lid | `/sys/class/power_supply/axp2202-battery/hallkey` (SP models) |
| FB blank | `/sys/class/graphics/fb0/blank` |
| HDMI state | `/sys/class/switch/hdmi/cable.0/state` (platform.c) / `/sys/class/extcon/hdmi/cable.0/state` (keymon) — extcon ✔ confirmed |
| WiFi state | `/sys/class/net/wlan0/operstate` ✔ confirmed (wlan0 exists) |
| Brightness | `/dev/disp` ioctl `DISP_LCD_SET_BRIGHTNESS` = `0x102`, arg `{0, raw 0-255, 0, 0}` |
| Volume | `amixer sset 'lineout volume' N%` ✔ control confirmed |
| Model string | `strings /mnt/vendor/bin/dmenu.bin \| grep ^RG` → `RGXX_MODEL` env |
| Headphone jack | `/sys/module/snd_soc_sunxi_component_jack/parameters/jack_state` (was noted "always 0" — needs re-test on 2026 firmware) |

- evdev button codes (from old platform.c, `/dev/input/event1` = ANBERNIC-keys):
```
UP 103  DOWN 108  LEFT 105  RIGHT 106          (d-pad also on ABS_HAT 16/17)
A 304   B 305     X 307     Y 306
START 311  SELECT 310  MENU 312 (alt 354)
L1 308  L2 314  L3 313    R1 309  R2 315  R3 316
PLUS 115 (vol+)  MINUS 114 (vol-)  POWER 116
Analog: LSX=ABS_Z(2) LSY=ABS_RX(3) RSX=ABS_RY(4) RSY=ABS_RZ(5), raw 0..4096, scale *32767/4096
```
  (ABS bits `0x3003c` on the live device confirm axes 2,3,4,5 + hats 16,17.)
- Panel geometry per model (old platform.h):
  - default (RG35XX+/H/SP, RG40XX*): 640×480
  - RG34xx: 720×480 (`is_rg34xx`)
  - RGcubexx: 720×720 (`is_cubexx`, only overscan-capable device)
  - RG28XX: physically 480×640 portrait; fb modes contain `480x640`; old SDL-malifbdev-rot handled rotation inside SDL, `rotate=3` fallback via `SDL_RenderCopyEx` when mode reports portrait
- Panel-fix DTB timing table (SKU fingerprints, `lcd_dclk_freq-lcd_ht-lcd_vt`):
  `24-770-526`=35xx2024/Plus, `24-770-528`=35xxH, `24-586-686`=28xx, `24-770-525`=35xxSP,
  `24-770-522`=40xxH, `36-812-756`=CubeXX, `26-820-536`=34xx

## Old-port scope note
The rg35xxplus material above is a **hardware-paths reference only** (sysfs paths,
evdev codes, boot hijack, panel table). Its software architecture (SDL_Renderer
compositing, 32-bit build, SDL1 warm-up shim, pre-shader NextUI APIs) is obsolete —
the port clones **tg5040** (GLES 3.x shader pipeline via `generic_video.c`, 64-bit)
and only borrows hardware constants from the old code.

## Open items to verify when devices are in hand (tracked in 08-testing)
1. ~~ELF class + GLES version of the Mali blob~~ **RESOLVED ✔**: 64-bit, ES 3.2, r20p0, fbdev winsys (see GPU section).
2. `DISP_LCD_SET_GAMMA_TABLE` (0x10b) ioctl support on H700 4.9 disp2 (for displaycal) — write a 10-line test program.
3. `hallkey` presence on RG34XXSP.
4. RG28XX fb0 reports `480x640` on current firmware.
5. Whether stock `dmenu.bin` model string (`strings … | grep ^RG`) is still the best model detector on 2026 firmware, vs `axp2202-battery/display_id` / DTB timing.
6. Headphone jack detection path on 2026 firmware (old note said jack_state never changes).
7. WiFi behavior across suspend/resume (this probe session: device didn't come back on WiFi after `rtcwake` suspend until manual wake — determine if wlan needs a post-resume kick like tg5040's suspend script does).
