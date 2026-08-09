# 01 — Toolchain & Build System

Build architecture, dependency decisions, and cross-toolchain lessons. Test results
live in [08](08-testing-status.md); promotion requirements live in
[09](09-roadmap.md).

## How the h700 build works

The CI-supported single-platform staging sequence is `make setup` followed by
`make h700`. (`make all` builds every configured platform; adding `PLATFORM=h700`
does not narrow it.) H700 builds inside the dedicated
**`ghcr.io/loveretro/h700-toolchain`** image (`LoveRetro/h700-toolchain`). Same arch
(aarch64/cortex-a53), same `-mcpu=cortex-a53` tuning, and the image's glibc (2.33,
via the TG5040 SDK sysroot) is older than the target's (Ubuntu 22.04, glibc 2.35) —
forward-compatible by construction. This was empirically validated early in the port
(a cross-built displaycal.elf ran unmodified on RG40XXV stockmod and an RG34XXSP
running Knulli) and has held up in production. The image is a sibling of
`tg5040-toolchain` (same GCC 8.3 + SDK recipe) with its own GHCR lifecycle so H700
no longer shares the tg5040 container tag.

Integration points:
- Root `makefile`: `PLATFORMS = tg5050 tg5040 h700`; the `make h700` dispatcher runs
  the full `common` staging target for H700, while `make shell` passes
  `PLATFORM=$(PLATFORM)` through (and `makefile.toolchain` injects
  `-e PLATFORM -e UNION_PLATFORM` into the container).
- `makefile.toolchain`: `PLATFORM=h700` → clone `toolchains/h700-toolchain/`, image
  `ghcr.io/loveretro/h700-toolchain:latest` (same pattern as tg5040/tg5050; no remap).
- `workspace/h700/platform/makefile.env`: tg5040-derived flags (`-mcpu=cortex-a53`,
  `SDL = SDL2`, `GL = GLES`), with the in-tree SDL2 prefix first in include/lib order.
- `workspace/h700/makefile`: platform-local `early` stages prebaked SDL2 from the
  image and builds other deps (NextCommander, boot shim) before the apps.
- The full tg5040 core list (28 cores + patches) builds unchanged — same arch, same tuning.

### Never pass the platform as a `make` argument to the guest build

See the note above the `build` target in `makefile.toolchain`. A command-line variable is
recorded in `MAKEOVERRIDES` and overrides same-named assignments in *every* recursive
make, including the `PLATFORM = libretro` that picodrive and pcsx_rearmed set in their
picoarch-derived build system. Their libretro frontend objects then never reach `OBJS`,
and because the link uses `-flto`, a version script that keeps only `retro_*` global,
and `--gc-sections`, nothing is reachable: the linker emits a ~10KB stub that segfaults
as soon as minarch resolves `retro_init`. Upstream is unaffected — it invokes a bare
`make` in the container and lets `UNION_PLATFORM` flow through the environment, where a
makefile assignment still wins. The h700 branch introduced the command-line form in
`d4fc5b51`, which broke picodrive (worked around per-core in `04a1d62d`) and shipped a
stub PS1 core for h700, tg5040, and tg5050 in the 20260720 and 20260724 builds
([#19](https://github.com/pvaibhav/NextUI/issues/19)). The guest invocations now match
upstream and the per-core workarounds are gone.

A stub core is silent — it links, installs, and only fails when minarch resolves
`retro_init` — so if a core is ever suspected, check it directly rather than by size:

```
nm -D workspace/<platform>/cores/output/<core>_libretro.so | grep retro_api_version
```

### Prebaked SDL2 (h700-toolchain `PREFIX_LOCAL`)
On tg5040 the stock OS supplies runtime SDL2; on H700 we ship our own. The
**h700-toolchain** image builds **`JohnnyonFlame/SDL-malifbdev-rot`** (commit
`d4a7d7503524cc469fe775242f3f925d4dd56c88` + `support/sdl2-h700.patch`) into
`PREFIX_LOCAL=/opt/nextui`. NextUI's `early` target only stages those libs (plus
SDK `SDL2_image`/`ttf`, tinyalsa, libpng12) into `other/sdl2/output` for packaging.
`makefile.env` still prefers `PREFIX_LOCAL` for includes/libs/pkg-config.

Configure highlights (see `h700-toolchain/support/build-sdl2.sh`):

- `--enable-video-mali --enable-video-opengles`, x11/wayland/kmsdrm disabled.
  The build **asserts `#define SDL_VIDEO_DRIVER_MALI 1` in the generated
  `SDL_config.h`** right after configure — a silent fallback to the dummy driver
  produces a black screen much later, so fail fast here.
- `--enable-alsa --enable-alsa-shared` — ALSA loaded via **dlopen at runtime**, not
  direct-linked. This is load-bearing, not an optimization (see pitfall #1 below).
- `--enable-loadso --enable-filesystem` — loadso is required for GL context creation
  and for alsa-shared/image-shared dlopen to work at all. (An early draft disabled
  both; don't.)
- No udev (`SDL_JOYSTICK_DISABLE_UDEV=1` is also exported at runtime — see 03).
- **`support/sdl2-h700.patch`** (in h700-toolchain) restores joystick classification
  via `BTN_GAMEPAD` / `BTN_JOYSTICK` (the fork's Batocera patches removed heuristics
  in favor of udev).

### Runtime library bundling (`platform/makefile.copy`)
Bundle into `.system/h700/lib` only what the stock OS lacks or can't be trusted for:
`libSDL2*` (our custom build + matching SDL2_image/ttf), `libtinyalsa.so*`,
`libpng12.so*` (see pitfall #2). **Deliberately not bundled:** `libasound`
(dlopened from the device — bundling the SDK's copy caused the audio bug) and
`libUMP` (doesn't exist on Mali-G31 systems; an early build bundled it by cargo-cult).
Rule of thumb: `ldd` every shipped .elf against a clean stock rootfs; bundle exactly
the misses, nothing more.

The stock Anbernic OS also lacks the `curl` CLI used by NextUI's RetroAchievements
HTTP layer. The H700 build therefore builds pinned curl 8.21.0 with pinned OpenSSL
3.5.7 and musl 1.2.6 as a static aarch64 binary, verifies all source checksums and the
final linkage, and stages it at `.system/h700/bin/curl`. Using musl avoids static
glibc's runtime dependency on matching NSS modules for DNS. A private CA bundle is
staged at `.system/h700/etc/ssl/certs/ca-certificates.crt`; `launch.sh` sets
`CURL_CA_BUNDLE` to that path. Since NextUI's bin directory is first on `PATH`, this
works on stock OS without installing or replacing anything on TF1.

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

## Sysroot-mismatch pitfalls (lessons for future platform ports)

H700 originally built inside the tg5040 image and still uses that SDK as the
h700-toolchain sysroot. Cross-building against a *sibling platform's* rootfs works,
but every failure below came from exactly that gap. Check these first on any
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

## Decision: dedicated h700-toolchain (same SDK recipe for now)

The port first reused `tg5040-toolchain` in-process; the **pitfalls above are solved
in NextUI** (dlopen ALSA, direct GLES link, bundled libpng12). We now use a
**dedicated** `h700-toolchain` image/repo so H700 builds do not share the tg5040
container lifecycle, while keeping the proven GCC 8.3 + TG5040 SDK sysroot.

The image prebakes mali-fbdev SDL2 into `PREFIX_LOCAL` and does **not** rebuild
BlueZ (stock H700 BlueALSA/BlueZ at runtime — see 05 and 07). Optional later:
jammy-matched libasound in the sysroot to harden pitfall #1 further if direct
links reappear.

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
