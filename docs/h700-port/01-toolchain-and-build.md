# 01 — Toolchain & Build System

Build architecture, dependency decisions, and cross-toolchain lessons. Test results
live in [08](08-testing-status.md); promotion requirements live in
[09](09-roadmap.md).

## How the h700 build works

The CI-supported single-platform staging sequence is `make setup` followed by
`make h700`. (`make all` builds every configured platform; adding `PLATFORM=h700`
does not narrow it.) H700 has one build-system twist: **there is no dedicated h700
toolchain image — the build runs inside the existing
`ghcr.io/loveretro/tg5040-toolchain` image.** Same arch (aarch64/cortex-a53), same
`-mcpu=cortex-a53` tuning, and the image's glibc (2.33) is older than the target's
(Ubuntu 22.04, glibc 2.35) — forward-compatible by construction. This was empirically
validated before any code was written (a tg5040-built displaycal.elf ran unmodified on
RG40XXV stockmod and an RG34XXSP running Knulli) and has held up in production.

Integration points:
- Root `makefile`: `PLATFORMS = tg5050 tg5040 h700`; the `make h700` dispatcher runs
  the full `common` staging target for H700, while `make shell` passes
  `PLATFORM=$(PLATFORM)` through (and `makefile.toolchain` injects
  `-e PLATFORM -e UNION_PLATFORM` into the container).
- `makefile.toolchain`: maps h700 → the tg5040 image.
- `workspace/h700/platform/makefile.env`: tg5040-derived flags (`-mcpu=cortex-a53`,
  `SDL = SDL2`, `GL = GLES`), with the in-tree SDL2 prefix first in include/lib order.
- `workspace/h700/makefile`: platform-local `early` target builds the external deps
  (SDL2, below) before the apps.
- The full tg5040 core list (28 cores + patches) builds unchanged — same arch, same tuning.

### In-tree SDL2 (the one real gap in the shared image)
On tg5040 the stock OS supplies runtime SDL2; on H700 we ship our own. The platform
`early` target clones **`JohnnyonFlame/SDL-malifbdev-rot`**, **pinned to commit
`d4a7d7503524cc469fe775242f3f925d4dd56c88`**, builds it aarch64 and installs into a local
prefix that `makefile.env` puts first. Configure highlights (see `workspace/h700/makefile`):

- `--enable-video-mali --enable-video-opengles`, x11/wayland/kmsdrm disabled.
  The makefile **asserts `#define SDL_VIDEO_DRIVER_MALI 1` in the generated
  `SDL_config.h`** right after configure — a silent fallback to the dummy driver
  produces a black screen much later, so fail fast here.
- `--enable-alsa --enable-alsa-shared` — ALSA loaded via **dlopen at runtime**, not
  direct-linked. This is load-bearing, not an optimization (see pitfall #1 below).
- `--enable-loadso --enable-filesystem` — loadso is required for GL context creation
  and for alsa-shared/image-shared dlopen to work at all. (An early draft disabled
  both; don't.)
- No udev (`SDL_JOYSTICK_DISABLE_UDEV=1` is also exported at runtime — see 03).
- **`patches/sdl2-h700.patch` is applied after clone** (stamped, like the
  NextCommander patch). The fork's "Batocera patches" commit removed the heuristic
  joystick classification from `SDL_EVDEV_GuessDeviceClass()` (Batocera classifies
  via udev), which — combined with our no-udev build — made `SDL_NumJoysticks()`
  permanently return 0 for the built-in pad. NextUI never noticed (it reads evdev
  raw — 03), but pure-SDL apps like NextCommander got no input at all. The patch
  restores classification via a `BTN_GAMEPAD`/`BTN_JOYSTICK` check (the upstream
  check wouldn't match anyway: the pad exposes no ABS_X/ABS_Y).

### Runtime library bundling (`platform/makefile.copy`)
Bundle into `.system/h700/lib` only what the stock OS lacks or can't be trusted for:
`libSDL2*` (our custom build + matching SDL2_image/ttf), `libtinyalsa.so*`,
`libpng12.so*` (see pitfall #2). **Deliberately not bundled:** `libasound`
(dlopened from the device — bundling the SDK's copy caused the audio bug) and
`libUMP` (doesn't exist on Mali-G31 systems; an early build bundled it by cargo-cult).
Rule of thumb: `ldd` every shipped .elf against a clean stock rootfs; bundle exactly
the misses, nothing more.

CI now applies that rule to the highest-risk h700 GUI binary:
`workspace/h700/check-settings-ldd.sh` runs `ldd` for `settings.elf` in an Ubuntu
22.04/Jammy arm64 runtime with `.system/h700/lib` first, and fails on unresolved
libraries or missing gio/glib linkage.

### Updater detection (multi-platform SD cards)
`skeleton/BOOT/common/updater` detects H700 via `grep -q sun50iw9
/proc/device-tree/model` **before** the `*"0xd03"*` cpuinfo case — H700's cpuinfo is
indistinguishable from other A53 platforms. On Anbernic the normal boot path doesn't go
through `updater` (our dmenu.bin calls `.tmp_update/h700.sh` directly), but correct
detection prevents mis-flash if a multi-platform card moves between devices.

## Toolchain-reuse pitfalls (lessons for future platform ports)

Cross-building in a *sibling platform's* image against a *different* target rootfs
works, but every failure below came from exactly that gap. Check these first on any
future stock-OS port:

1. **libasound symbol versioning → glitchy audio.** The tg5040 SDK's libasound has no
   symbol versioning. Direct-linking SDL against it made the dynamic linker resolve the
   *unversioned* legacy `ALSA_0.9` `snd_pcm_hw_params_set_*` symbols from the device's
   (versioned) libasound at runtime. Those have value semantics instead of
   pointer-in/out — `set_rate_near` silently clamped the codec to 192 kHz while SDL
   believed 32.768 kHz → sliced, glitchy audio that *almost* worked. Fix:
   `--enable-alsa-shared` (dlopen; dlsym always picks the default/current symbol
   version). General lesson: **for libraries that exist on the target, prefer dlopen
   over cross-linking whenever the SDK's copy may differ in symbol versioning.**
2. **dlopen'd deps have their own arch requirements.** SDL2_image dlopens
   `libpng12.so.0`. The stock H700 OS only has a *32-bit* copy (under
   `/mnt/vendor/lib`); dlopen fails silently → no PNG loading. We bundle the
   toolchain's 64-bit `libpng12.so.0`. Lesson: audit not just `ldd` output but the
   *dlopen list* of every shipped library (`strings *.so | grep '\.so'`).
3. **pkg-config lies across rootfs boundaries.** The tg5040 SDK's `glesv2.pc` links
   libUMP (a Utgard-GPU-era dependency that doesn't exist on Mali-G31/bifrost
   systems). Link `-lGLESv2 -lEGL` directly instead of trusting sibling-platform
   pkg-config for GPU libs.
4. **LTO + libretro cores.** The inherited `-flto` broke picodrive: the linker plugin
   dropped libretro glue objects. The h700 cores makefile disables LTO / filters
   linker-plugin flags for affected cores, and pins `override PLATFORM := libretro`
   for reproducible clean rebuilds.
5. **Target shell ≠ build assumptions.** Stock `/bin/sh` is **dash**. The bashism `&>`
   (used throughout the tg5040 pak scripts we cloned) parses under dash as
   "background the command and truncate a file named by the next word" — launch
   scripts exited instantly and games bounced back to menu. All h700 pak scripts use
   POSIX `> file 2>&1`. Grep any cloned script for bashisms before shipping.
6. **Shared-code makefile gates.** `workspace/all/minarch/makefile` gates features
   (RetroAchievements, CHD, SRM, libsamplerate) by platform name — a new platform
   must be added to those filter lists or minarch silently builds featureless (or not
   at all).

## Decision: reuse the tg5040 image instead of a dedicated H700 image

The reuse costs above are all *solved*, and the SDL2 build is pinned and cached. A thin
image `FROM tg5040-toolchain` pre-baking SDL2 + a jammy-matched libasound would remove
pitfall classes 1–3 structurally and speed CI — worth doing if the platform accumulates
more external deps. Bluetooth audio uses the stock H700 BlueALSA daemon, ALSA plugins,
SBC runtime, and BlueZ without copying them from the shared toolchain (see 05 and 07;
09 classifies the release implications).

## Build outputs

After `make setup && make h700`, the staging tree contains:
- `build/SYSTEM/h700/bin/*` — nextui.elf, minarch.elf, keymon.elf, batmon.elf, audiomon.elf,
  gametimectl.elf, syncsettings.elf, nextval.elf, show2.elf, settings.elf, clock, …
  (tg5040 list minus ledcontrol, which is gated to tg50x0; bootlogo.elf builds for
  h700 too and ships in `EXTRAS/Tools/h700/Bootlogo.pak`) plus `rfkill`
  (h700 builds its own minimal `/dev/rfkill` ioctl tool — stock rfkill may be absent)
- `build/SYSTEM/h700/lib/` — libmsettings.so, libbatmondb.so, libgametimedb.so, …
  plus the bundled SDL2/tinyalsa/libpng12 set above
- `build/SYSTEM/h700/cores/*.so` — full tg5040-parity core list
- `build/SYSTEM/h700/shaders/` — tg5040 `.glsl` set (ES 3.0, portable)
- `build/BOOT/common/h700.sh` (installer), `build/BASE/h700/dmenu.bin` (boot shim — see 02),
  NextCommander built via `patches/NextCommander-h700.patch`

The later `make special && make package` steps rename `build/SYSTEM` to
`build/PAYLOAD/.system`, move the updater tree to `.tmp_update`, and create the
release archives.
