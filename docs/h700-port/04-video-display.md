# 04 — Video, Display, Rotation, HDMI, DisplayCal

## Architecture recap
NextUI renders via `workspace/all/common/generic_video.c`: SDL2 window →
`SDL_GL_CreateContext` (requests **GLES 3.2** profile) → shader pipeline (shaders are
rewritten to `#version 300 es`, so ES 3.0 is the true floor) → `SDL_GL_SwapWindow`.
No fbdev blits, no SDL_Renderer in the hot path. Whatever platform provides a working
SDL2 + EGL/GLES stack gets the entire NextUI feature set (shaders, overlays, effects,
scrolling text, screenshots) for free.

## The H700 graphics stack (the real porting work)

- Kernel 4.9 BSP: **no DRM/KMS**. Display = Allwinner disp2 (`/dev/disp` + fbdev).
- GPU: Mali-G31 MP2, `mali_kbase` r20p0 kernel driver + vendor blob userspace.
  **Verified on device:** blob `libmali.so.0.20.0` is 64-bit aarch64, reports
  **`OpenGL ES 3.2 v1.r20p0-01rel0`** (matches kernel kbase r20p0), and is built with
  the **fbdev EGL winsys** — every assumption in this doc is confirmed hardware fact.
- Therefore SDL2 uses the **Mali/fbdev EGL winsys** (EGL `fbdev` native window),
  like muOS/Knulli's SDL on these devices.
- Extra proof: the **stock OS's own `/usr/lib/libSDL2-2.0.so.0.12.0` is 64-bit and
  compiled with exactly the `mali` video driver** (only `mali` + `dummy` backends).
  The stack we're building is the one the device already runs. During bring-up, the
  stock libSDL2 can even serve as a temporary crutch (it's SDL 2.0.12 — too old to
  ship, but fine for validating our binaries before the custom SDL is done).

### Custom SDL2 (bundled in `.system/h700/lib`)
Start from **`JohnnyonFlame/SDL-malifbdev-rot`** (what the old port shipped: SDL 2.28.5,
`--enable-video-mali`, with built-in **rotation** support — written specifically for
the RG28XX/RGcubexx generation). Tasks:
1. Build it **aarch64** against the jammy sysroot + device Mali blob (old port built it 32-bit).
2. Confirm `SDL_WINDOW_OPENGL` + `SDL_GL_CreateContext` with ES 3.2/3.0 works on the
   mali video driver path (the old port used SDL_Renderer on top; NextUI drives GL
   directly — the mali backend supports EGL contexts natively, this is its whole point).
3. Audio: keep ALSA backend enabled (`--enable-alsa`), everything else off (matches
   old configure: no kmsdrm/x11/wayland/pulse/dbus).
4. Evaluate rebasing the mali-fbdev patch onto SDL 2.30.x if NextUI depends on newer
   SDL APIs — check what SDL version the tg5040 SDK ships and what `generic_video.c`
   + `api.c` actually require; 2.28.5 is likely sufficient. (Alternative donor with
   the same backend: Knulli's buildroot patchset for h700.)

Phase 0 gate (see 01): a 200-line raw EGL/GLES test binary proves blob + fbdev winsys
+ ES version before any SDL work. If the blob's EGL doesn't accept fbdev native
windows (unlikely — stock RA uses it), fall back to the blob variant muOS ships.

## Per-device geometry

| Device | Panel | FIXED_W×H | Notes |
|---|---|---|---|
| RG40XXV | 640×480 4:3 | 640×480 | reference target |
| RG34XXSP | 720×480 3:2 | 720×480 | `is_rg34xx` (same LCD as RG34XX) |
| RG28XX | **480×640 portrait** | 640×480 logical | needs rotation (below) |
| RGcubexx | 720×720 | 720×720 | later; only overscan device |

UI scale: these are ~half the resolution of tg5040 (1280×720/1024×768). `FIXED_SCALE 2`
and `MAIN_ROW_COUNT 6` follow the old port. Audit current NextUI UI code for
assumptions introduced since the 480p platforms were dropped (font sizes, pill
sprites, quick switcher) — run the whole UI at 640×480 in the `desktop` platform
first if it supports arbitrary resolution, or budget polish time here. **This is a
real risk area: NextUI hasn't rendered at 480p for ~2 years.**

## RG28XX rotation (deferred to phase 2, design now)

The panel scans portrait 480×640; fb0 mode will report `480x640`. Options, in order
of preference:
1. **Inside SDL (malifbdev-rot)**: the backend was patched precisely for this — it
   presents a landscape-logical display and rotates in the blit/flip. If it works with
   GL contexts (verify! the rot patch may only cover the non-GL blitter path), NextUI
   needs zero changes.
2. **GL-level rotation in generic_video.c**: add a platform hook (e.g.
   `PLAT_getDisplayRotation()` returning 0/90/270) applied as a final rotation in the
   present pass (rotate the output quad / swap w↔h of the backbuffer viewport). Clean,
   ~contained change; also benefits any future rotated device.
3. Allwinner DE rotation via `/dev/disp` (the disp2 driver has a rotation/smart-color
   module on some BSPs) — investigate `dispdbg`; least portable, likely dead end.

Plan for (2) as the durable solution, try (1) first since the code exists.
Everything else (input is unrotated, touch none) is unaffected.

## HDMI out

Old port: `HAS_HDMI`, 1280×720 output, monitored by `hdmimon.sh` (recover:
`git show 8cd78866:skeleton/SYSTEM/rg35xxplus/bin/hdmimon.sh`) using
`/sys/kernel/debug/dispdbg` (`switch1 4 10 ...`) + `fbset`, then restarting the UI with
`hdmi_export.sh` sourced. Hotplug state: `/sys/class/extcon/hdmi/cable.0/state`
(confirmed present).

Current tg5040 NextUI: check how/if it handles HDMI (TrimUI Brick HDMI support in
NextUI is limited). **Scope decision: HDMI = stretch goal, phase 3.** The plumbing is
understood and documented; don't block the handheld experience on it. Until then:
`PLAT` reports no HDMI; NextUI treats display as fixed.

## DisplayCal (white-point correction) — expected to port 1:1

- Lives in `workspace/all/common/displaycal.{c,h}`, compiled into `libmsettings.so` +
  standalone `displaycal.elf`; UI gated at runtime on model name in `settings.cpp`.
- Mechanism: 256-entry RGB gamma LUT via `/dev/disp` ioctls `DISP_LCD_SET_GAMMA_TABLE
  (0x10b)` / `GAMMA_CORRECTION_ENABLE (0x10c)` / `DISABLE (0x10d)`.
- H700 has the same disp2 driver generation (attr dir matches TG5040 nearly 1:1, incl.
  `color_temperature`). The user reports displaycal-class functionality already works
  fine on RG XX devices. **Validation (Phase 0/2):** 20-line test — set a strongly
  tinted LUT, confirm visible effect, confirm survival across suspend/resume.
- Work items: add h700 to the displaycal build (`libmsettings` makefile), extend the
  model gate in `settings.cpp` to Anbernic models, add per-panel default gain presets
  (measure per device; start neutral 100/100/100).
- `enhance_*` attrs (bright/contrast/saturation) exist too — same as tg5040; whatever
  NextUI exposes for those on tg5040 carries over.

## Boot splash
`show2.elf` (SDL) post-install; raw fb `dd` in the dmenu.bin shim pre-install (see 02).
Generate 640×480, 720×480, 480×640(rotated) variants of the splash assets — the old
port's `-r/-s/-w` suffix scheme, selected by fb0 mode.
