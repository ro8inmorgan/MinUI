# 04 — Video, Display, Rotation, HDMI, DisplayCal

## Architecture (worked exactly as designed)

NextUI renders via `workspace/all/common/generic_video.c`: SDL2 window →
`SDL_GL_CreateContext` → GLES 3 shader pipeline → `SDL_GL_SwapWindow`. The H700
platform provides SDL2 + a working EGL/GLES stack and gets the entire feature set
(shaders, overlays, effects, scrolling text, screenshots) for free. **Confirmed on
hardware**: all shipped `.glsl` shaders, overlays, and effects run on the Mali-G31 at
full speed at 640×480.

The stack:
- Kernel 4.9 BSP, no DRM/KMS. Display = Allwinner disp2 (`/dev/disp` + fbdev).
- Mali-G31 MP2, kbase r20p0 + vendor blob (64-bit, ES 3.2, fbdev EGL winsys — 00).
- **Custom SDL2** from `JohnnyonFlame/SDL-malifbdev-rot`, pinned commit
  `d4a7d750…`, built aarch64 in-tree (01), `SDL_VIDEODRIVER=mali` exported by
  launch.sh. SDL 2.28-era — sufficient for current NextUI; no 2.30 rebase was needed.
- GLES linked directly (`-lGLESv2 -lEGL`), **not** via the SDK's pkg-config (libUMP
  trap, see 01). `generic_video.c` gained hard error checks: SDL init / window /
  renderer / GL-context failures now `exit(1)` instead of limping on — on a device
  with no display fallback, failing loudly into the bounded crash-restart loop beats
  a black screen; the release path powers off after the crash limit.

## Per-device geometry

| Device | Panel | FIXED_W×H | Status |
|---|---|---|---|
| RG40XXV | 640×480 4:3 | 640×480 | ✅ shipped, tested |
| RG34XXSP | 720×480 3:2 | 720×480 | ✅ Battery, Game Tracker, Input, Clock, Settings, Files, keyboard, box art, game switcher and in-game menus tested-good; resolution-specific overlays untested |
| RG28XX | 480×640 portrait | 640×480 logical | ✅ user-tested — driver-level rotation, UI + game scaling verified (below) |
| RGcubexx | 720×720 | 720×720 | wired (`is_cube`), untested |

### 480p UI — mostly fine, polish pass pending
These panels are ~half the resolution of tg5040 (1280×720 / 1024×768); NextUI hadn't
rendered at 480p for ~2 years. Real-use verdict on RG40XXV and RG34XXSP: **no systemic
breakage — OK for alpha**. The broad 720×480 surface sweep is now clean, including the
keyboard used for RetroAchievements credentials. Screenshots and resolution-specific
overlays remain untested; no suitable 720×480 overlay asset was available. Related but
distinct: the Input tester is now device-aware, the dead
display controls are hidden/verified, and Fn-switch settings are gated off on h700.
The broader capability sweep remains a useful regression check (09-roadmap #8).

## RG28XX rotation — validated: driver-level only (2026-07-12)

Hardware testing answered both open questions, and the answer retired one of the
two planned layers:

1. **SDL level (the only layer)**: launch.sh exports `SDL_ROTATION=1` when
   `DEVICE=rg28xx`, consumed by the malifbdev-rot driver. It **does** cover the GL
   context path — the entire 640×480 landscape frame is rotated onto the portrait
   panel at present time. The app-side coordinate space is plain 640×480 landscape;
   no NextUI code needs to know the panel is rotated.
2. **GL level — removed.** `platform.c` initially also set `should_rotate =
   is_rg28xx`, feeding `generic_video.c`'s dst-rect w/h swap in
   `setRectToAspectRatio()`. The feared double-rotation was real: the UI path
   doesn't go through that function so menus looked fine, but minarch scaling
   broke — Aspect produced an undersized rect fitting neither axis (480×360 inside
   640×480), and Fullscreen stretched into a 480×640 rect that filled only the left
   75% of the screen with the bottom clipped. Removing the flag fixed both modes,
   user-verified on hardware (`d738014`). A NOTE in `PLAT_initPlatform` documents
   why `should_rotate` must stay 0. (That flag was originally minarch's
   TATE/vertical-arcade content-rotation hook — a feature since removed — not a
   panel-orientation flag; h700 is the only platform that ever set it.)

One consequence for the Bootlogo pak: the bootloader blits `bootlogo.bmp`
panel-native (unrotated), so the 480×640 presets are authored 90° CCW in file
space. The pak now rotates previews 90° CW on the RG28XX
(`BOOTLOGO_PREVIEW_ROTATE_CW` in platform.h, default-off for other devices) so the
carousel shows what the panel will actually display at boot; apply itself was
already correct (user-verified: applied logo renders upright at power-on).

## HDMI — detection wired, output switching not

- `GetHDMI()` probes `/sys/class/extcon/hdmi/{state,cable.0/state}` so hotplug
  *detection* works (hdmimon/rumble-skip logic can fire).
- `SetHDMI()` is an **empty no-op** — no mode switch / fb re-init / UI restart. Full
  HDMI out remains a stretch goal. The known mechanism, if ever needed: old port's
  `hdmimon.sh` via `/sys/kernel/debug/dispdbg` (`switch1 4 10 …`) + `fbset`, 1280×720,
  audio on ALSA card 2 (`ahubhdmi`). Recover with
  `git show 8cd78866:skeleton/SYSTEM/rg35xxplus/bin/hdmimon.sh`.

## DisplayCal — ported 1:1, tested ✅

Same `/dev/disp` gamma-LUT ioctls as tg5040 (`0x10b` set / `0x10c` enable / `0x10d`
disable). On RG40XXV: RGB gain sliders visibly act, persist, and **survive sleep and
game launch** (syncsettings.elf re-applies the LUT after resume — 06). `settings.cpp`
gained the Anbernic vendor + RG XX models and enables
colortemp/displaycal/mute/analog-stick/wifi/bt capability flags for h700.

**Reboot persistence — fixed 2026-07-09.** `InitSettings()` used to call
`applyDisplayCalDefaultsForDevice()` unconditionally *after* loading the persisted
`msettings.bin`, clobbering white point / RGB gains back to off/100/100/100 on every
boot (and every client attach). Now only the shm host seeds displaycal defaults, and
only when the file is missing or predates v11. Verified on RG40XXV: patched values
survive reboot. (tg5040 has the same latent bug, masked by its calibrated presets —
flagged as a separate task.)

**Per-model default presets — plumbed, not yet measured.** `displaycal.h` has a
preset per H700 model (RG28XX, RG34XX, RG34XXSP, RG35XX = Plus/H/2024 shared,
RG35XXSP, RG35XXPRO, RG40XXH, RG40XXV, RGCubeXX), all currently disabled/neutral
(100/100/100). Selection keys on `RGXX_MODEL` (exact-model string from stock
`dmenu.bin`; confirmed so far: `RG28xx`, `RG34xx`, `RG34xxSP`, `RG40xxV`,
`RGcubexx` — RG35xx-family and RG40xxH strings matched by prefix until confirmed)
with `DEVICE` fallback, in both h700 libmsettings and the settings app's
reset-to-defaults. launch.sh now maps `RG35xx*` → `DEVICE=rg35xx` instead of lumping
the 35xx family into rg40xx. Calibrating a panel later = editing numbers in
displaycal.h only. Default brightness on h700 is 4 (tg5040 Brick keeps 2).

**What actually works on RG XX panels:** LCD backlight brightness, color temperature,
white-point correction (displaycal), RGB tuning. The `enhance_*` display controls
(contrast/saturation/exposure) **do nothing on RG XX** — the sysfs attrs exist (00)
yet have no visible effect — so since 2026-07-09 they are hidden on h700 via the
`DeviceInfo` capability gates in settings.cpp (including the mute-toggle variants),
visually verified on RG40XXV.
The libmsettings plumbing remains (harmless no-ops; the shared API keeps the
symbols), and syncsettings still "restores" them on resume — a cosmetic cleanup at
most.

## Alpha/tinted-bitmap blits — resolved (it was libpng all along)

During bring-up, graphical glitches were blamed on main's alpha-blending work; a
compat experiment (`GFX_needsBitmapBlendCompat()` gated in shared `api.c`) was
committed and reverted 16 minutes later, and the branch stayed pinned on the
pre-alpha graphics stack. **The real root cause turned out to be the missing 64-bit
`libpng12` bundle** — SDL2_image dlopens libpng, the stock OS only has a 32-bit copy,
and the silent dlopen failure corrupted image loading (fixed in `510d3bb1`, see
pitfall #2 in 01). The alpha-blending code on main was never at fault.

Consequence: the h700 branch was **rebased onto main with the alpha-blending work
included** (done 2026-07-09). `workspace/all/` carried zero net change from the
reverted experiment, so the rebase was clean on that front. Game switcher and box-art
rendering are clean at 720×480; screenshots and resolution-specific overlays remain to
be checked. Upstream main has since advanced, so rebase once more before final merge.

## Bootlogo preset previews — resolved (2026-07-13)

The 640×480 catalog works on RG40XXV. The RG34XXSP "no visible presets" issue is
fixed: driving the pak remotely over SSH on the SP with the current branch build
showed all 23 `720x480/` presets loading and the carousel rendering correctly
(verified via framebuffer capture); user-confirmed working. The failing binary was
the alpha1.1-era `bootlogo.elf` (built from a pre-commit working tree); the exact
defect in that binary was never pinned because current source no longer reproduces
it. To keep this class of failure diagnosable, `bootlogo.c` now logs the preset
search path, per-file `IMG_Load` failures, and the final count to the pak's
`log.txt`, renders the searched path on screen when nothing loads, and guards
scroll/apply against an empty preset list (apply previously dereferenced a NULL
array). RG28XX previews are rotated to boot orientation (see rotation section
above). The 720×720 path still needs hardware testing (no cube available).

## Boot splash

Deviation from plan: instead of per-panel raw-BMP `dd` assets, the shim ships
`fbsplash` — a dependency-free fb0 text renderer (02). Post-install, SDL-based
`show2.elf` handles rich splash/progress as on other platforms. No per-resolution
splash assets to maintain.
