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
  with no display fallback, failing loudly into the crash-restart loop (which brings
  up SSH) beats a black screen.

## Per-device geometry

| Device | Panel | FIXED_W×H | Status |
|---|---|---|---|
| RG40XXV | 640×480 4:3 | 640×480 | ✅ shipped, tested |
| RG34XXSP | 720×480 3:2 | 720×480 | wired (`is_rg34xx`), untested |
| RG28XX | 480×640 portrait | 640×480 logical | rotation plumbed, untested (below) |
| RGcubexx | 720×720 | 720×720 | wired (`is_cube`), untested |

### 480p UI audit — still open
These panels are ~half the resolution of tg5040 (1280×720 / 1024×768); NextUI hadn't
rendered at 480p for ~2 years. `FIXED_SCALE 2` + `MAIN_ROW_COUNT 6` work, and general
browsing looks right on RG40XXV, but no systematic audit of fonts / pills / quick
switcher / long-text layouts at 640×480 (or 720×480) has been done. See 09-roadmap.

## RG28XX rotation — implemented, unvalidated

Both planned layers were plumbed:
1. **SDL level**: launch.sh exports `SDL_ROTATION=1` when `DEVICE=rg28xx`
   (consumed by the malifbdev-rot driver — rotating in the backend was the repo's
   whole reason to exist).
2. **GL level**: `platform.c` sets `should_rotate = is_rg28xx`, feeding
   `generic_video.c`'s existing rotation handling (dst-rect w/h swap in present).

Unknown until hardware testing: whether the malifbdev-rot patch covers the *GL
context* path (the original concern was it might only rotate the non-GL blitter),
and whether the two layers interact correctly (both active could double-rotate).
Treat the whole path as unverified.

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
game launch** (syncsettings.elf re-applies the LUT after resume — 06). The `enhance_*`
attrs (contrast/saturation/exposure) are exposed through settings just like tg5040;
`settings.cpp` gained the Anbernic vendor + RG40XX/RG34XX/RG28XX/RGCubeXX models and
enables colortemp/displaycal/mute/analog-stick/wifi/bt capability flags for h700.
Per-panel default gain presets: not yet measured (neutral defaults).

## Known unresolved: alpha/tinted-bitmap blits

An attempt to normalize the asset sheet and force alpha-blend blits for H700's
SDL2/SDL_image behavior (`GFX_needsBitmapBlendCompat()` gated in shared `api.c`) was
committed and **reverted 16 minutes later** — it regressed the known-good rendering.
The branch deliberately stays on the "pre-alpha graphics stack"; `workspace/all/`
carries **zero net change** from the experiment. Symptom class to look for when
revisiting: bitmaps whose alpha should tint/blend rendering opaque (or vice versa)
where tg5040 renders correctly. Root cause not yet established — likely a pixel-format
or blend-mode default difference in our SDL2/SDL2_image build. See 09-roadmap.

## Boot splash

Deviation from plan: instead of per-panel raw-BMP `dd` assets, the shim ships
`fbsplash` — a dependency-free fb0 text renderer (02). Post-install, SDL-based
`show2.elf` handles rich splash/progress as on other platforms. No per-resolution
splash assets to maintain.
