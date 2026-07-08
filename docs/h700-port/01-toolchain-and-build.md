# 01 — Toolchain & Build System Integration

## Goal
`make PLATFORM=h700 shell` / `make all` builds an `h700` platform exactly like tg5040 does
today: docker image pulled/built from `toolchains/h700-toolchain`, workspace mounted, all
apps + cores cross-compiled, packaged into `MinUI.zip`.

## Decision: 64-bit aarch64 build (not 32-bit like the old port)

The old rg35xxplus port was 32-bit (`arm-buildroot-linux-gnueabihf`) because the
2023-era stock OS was 32-bit. The 2026 stock OS is **Ubuntu 22.04 arm64 with glibc
2.35**, and the tg5040 platform is already aarch64/cortex-a53. Building 64-bit means:

- `workspace/h700/platform/makefile.env` is a copy of tg5040's (`-mcpu=cortex-a53 -flto`, `SDL = SDL2`, `GL = GLES`)
- The entire tg5040 core list builds unchanged (same arch, same tuning)
- No NEON/hard-float 32-bit toolchain maintenance
- Gate (Phase 0): confirm the device's Mali blob `/usr/lib/libEGL.so` is 64-bit.
  If it unexpectedly is not, fall back to shipping a known-good 64-bit H700 blob
  (the same blob muOS/Knulli use on this SoC) in `.system/h700/lib` — the kernel
  side (`mali_kbase` on 4.9) accepts both; blob and kernel driver version must match
  (check `dmesg | grep mali` for the kbase version, expect r16p0-style for G31).

## Toolchain: REUSE the tg5040 docker image (no new toolchain for bring-up)

**Empirically validated:** a tg5040-toolchain-built `displaycal.elf` runs unmodified
on the RG40XXV (stockmod) *and* on an RG34XXSP running Knulli. Same arch, same
`-mcpu=cortex-a53` tuning, and the tg5040 toolchain's glibc (2.33) is older than the
targets' (stock 2.35, Knulli ≥2.33) — forward-compatible by construction.

So: **`PLATFORM=h700` builds inside the existing `ghcr.io/loveretro/tg5040-toolchain`
image.** Implementation: `makefile.toolchain` derives the image name from
`$(PLATFORM)` — add a small override (e.g. `TOOLCHAIN_NAME ?= $(PLATFORM)` with
`h700: TOOLCHAIN_NAME=tg5040`, or an `IMAGE_OVERRIDE` var) rather than a new repo.

What the tg5040 image already provides for h700:
- aarch64 cross GCC + cortex-a53 flags — apps and all 28 cores build as-is
- SDK sysroot with nearly every NextUI dep: SDL2_image/ttf, sqlite, libsamplerate,
  libzip, curl/ssl, zlib/bz2/lzma/zstd/lz4, tinyalsa, GLES/EGL link stubs (runtime
  symbols come from the device's libmali — standard ABI, link stubs are fine)
- all host tools the cores makefile needs

The one real gap — **SDL2 itself**: on tg5040 the stock OS supplies runtime SDL2; on
H700 we ship our own mali-fbdev SDL2 (see 04) and must compile NextUI against *its*
headers/libs, not the SDK's. Solve it the way the old rg35xxplus port did: build the
custom SDL2 in-tree under `workspace/h700/other/sdl2/` via the platform `early`
target, install into a local prefix inside the container, and have
`workspace/h700/platform/makefile.env` put that prefix first in include/lib/pkg-config
order. Bundle the resulting `libSDL2*.so` (+ matching-version SDL2_image/ttf if the
SDK's are ABI-incompatible with SDL 2.28) into `.system/h700/lib`.

Runtime-lib rule of thumb: anything linked from the SDK sysroot that isn't guaranteed
on the H700 stock OS gets bundled into `.system/h700/lib` (NextUI already does this on
tg5040 for samplerate/zip/etc. — extend the list; `ldd` every shipped .elf against a
clean stock rootfs as a CI-able check).

**Dedicated `h700-toolchain` image: deferred, optional.** If/when divergence grows
(different SDL patches, extra deps like bluez-alsa, wanting jammy's exact glibc), a
thin image `FROM ghcr.io/loveretro/tg5040-toolchain` that pre-bakes the SDL2 build +
extras is the natural next step — cheap to add later, not a prerequisite. Revisit
after Phase 3.

## Repo integration checklist

- [ ] `makefile`: add `h700` to `PLATFORMS` (`PLATFORMS = tg5050 tg5040 h700`)
- [ ] `makefile.toolchain`: image-name override so `PLATFORM=h700` uses the tg5040 image (no `toolchains/h700-toolchain/` dir needed for now)
- [ ] `workspace/h700/` — new platform dir (contents defined in docs 03–07):
  ```
  platform/{platform.c,platform.h,makefile.env,makefile.copy}
  libmsettings/{msettings.c,msettings.h,makefile}
  keymon/{keymon.c,makefile}
  cores/makefile
  install/{boot.sh,update.sh,logo.png}
  boot/  (dmenu.bin builder — see 02)
  makefile  (platform-local `early`/`all` targets, modeled on tg5040's)
  ```
- [ ] `workspace/makefile`: tg5040 has special-cased steps (`rfkill`, `btmanager`, `poweroff_next`) under `ifeq ($(PLATFORM), tg5040)`. Add an h700 branch only for what h700 actually needs (likely `rfkill` only; plain `poweroff` works on systemd — verify — and bluez is modern already, no `btmanager` needed).
- [ ] `skeleton/SYSTEM/h700/` + `skeleton/EXTRAS/Tools/h700/` + `skeleton/BOOT/` additions (see 02)
- [ ] `skeleton/BOOT/common/updater`: add H700 detection **before** the `*"0xd03"*` case (H700 cpuinfo also matches 0xd03!):
  ```sh
  if grep -q sun50iw9 /proc/device-tree/model 2>/dev/null; then PLATFORM="h700"; fi
  ```
  (or match `*"sun50iw9"*` on `cat /proc/device-tree/model` — do NOT rely on /proc/cpuinfo which is indistinguishable from zero28)
  Note: on Anbernic the H700 boot path doesn't go through `updater` at all (our
  `dmenu.bin` calls `.tmp_update/h700.sh` directly, see 02), but keeping `updater`
  correct costs one line and prevents mis-detection if a multi-platform card is moved
  between devices.
- [ ] `github/` CI workflows: add h700 to the build matrix (mirror what exists for tg5040)

## Build outputs (parity with tg5040)

`make PLATFORM=h700 all` must produce inside `build/`:
- `.system/h700/bin/*` — `nextui.elf, minarch.elf, keymon.elf, batmon.elf, audiomon.elf, gametimectl.elf, syncsettings.elf, nextval.elf, show2.elf, settings.elf(=minput?), clock, ledcontrol …` (same list as tg5040; drop what doesn't apply, see per-doc notes)
- `.system/h700/lib/` — `libmsettings.so, libbatmondb.so, libgametimedb.so, libsamplerate…` **plus `libSDL2-2.0.so.0`, `libSDL2_image`, `libSDL2_ttf`** (unlike tg5040, the stock OS SDL2 2.0.12 has no usable GLES video backend for us — we bundle our own; LD_LIBRARY_PATH in launch.sh puts our lib dir first)
- `.system/h700/cores/*.so` — same core list as tg5040
- `.system/h700/shaders/` — copy of tg5040's `.glsl` set (they're ES 3.0, portable)
- `.tmp_update/h700.sh` (installer), `dmenu.bin` (boot shim, see 02)

## Suggested first milestone (Phase 0 spike, ~1-2 days)

Before writing any platform code, validate the whole chain with throwaway binaries,
all inside the existing tg5040 toolchain container:
1. ~~hello world ABI check~~ **already proven** — user's tg5040-built displaycal.elf
   runs on RG40XXV stockmod and RG34XXSP/Knulli.
3. Tiny EGL/GLES probe (fbdev EGL native window via blob): create context, print
   `GL_VERSION`/`GL_RENDERER`, clear screen to a color. This single test de-risks the
   entire video stack (64-bit blob works, ES version confirmed, fbdev winsys works).
4. Build the custom SDL2, run an SDL window + `SDL_GL_CreateContext` + swap test.
5. Write `mem` to `/sys/power/state` from a test binary, wake with power button ✔ (already proven via rtcwake).
