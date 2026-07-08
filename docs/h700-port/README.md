# NextUI → Allwinner H700 (Anbernic RG XX) Port Plan

**Goal:** run NextUI on the Anbernic RG XX line (Allwinner H700) with feature parity
with the TrimUI Brick (tg5040) — including displaycal, WiFi/BT, and first-class
sleep leveraging the stock OS's excellent suspend — installed *on top of the latest
Anbernic stock OS*, no reflash, fully reversible.

**Initial targets:** RG40XXV and RG34XXSP. RG28XX (rotated panel) follows. CubeXX,
HDMI-out and the wider RG35XX family are follow-ups on the same platform code.

## Documents

| Doc | Contents |
|---|---|
| [00-device-facts.md](00-device-facts.md) | Ground truth: live probe results from RG40XXV + TG5040, old-port sysfs inventory, boot-chain analysis |
| [01-toolchain-and-build.md](01-toolchain-and-build.md) | h700-toolchain docker image, sysroot, repo/build integration |
| [02-boot-and-installer.md](02-boot-and-installer.md) | dmenu.bin hijack, installer, SD layouts, skeleton tree |
| [03-platform-layer.md](03-platform-layer.md) | `workspace/h700/`: platform.c/h, input, libmsettings, keymon, cores |
| [04-video-display.md](04-video-display.md) | SDL2+Mali/GLES stack, per-device geometry, RG28XX rotation, HDMI, displaycal |
| [05-audio.md](05-audio.md) | ALSA/SDL audio, volume/mute, jack, BT audio |
| [06-power-sleep-battery.md](06-power-sleep-battery.md) | Deep sleep, wake, lid (34XXSP), battery, governors, poweroff |
| [07-wifi-bluetooth.md](07-wifi-bluetooth.md) | wpa_supplicant strategy, bluealsa, parity checklist |
| [08-testing-and-rollout.md](08-testing-and-rollout.md) | Phase-0 spikes, validation matrix, phases & estimates |

## Why this port is very tractable (evidence-based)

1. **Same silicon family, same BSP generation as the reference platform.** H700 and
   the TG5040's A133P are both quad-A53 Allwinner SoCs on 4.9 BSP kernels with the
   same disp2 display driver (`/dev/disp`, identical `/sys/class/disp` attrs — verified),
   the same AXP2202-family PMIC with the *same* battery sysfs paths (verified), and
   `freeze mem` suspend (verified working by suspending the actual RG40XXV remotely).
2. **Same CPU arch and compatible glibc.** Stock RG OS is Ubuntu 22.04 arm64
   (glibc 2.35) vs tg5040's glibc 2.33 — tg5040-built aarch64/cortex-a53 binaries are
   ABI-compatible with the target. Cores list ports unchanged; no 32-bit toolchain
   resurrection needed (the old rg35xxplus port was 32-bit — we go 64-bit).
3. **NextUI's platform abstraction is clean.** Shared code (`workspace/all/`) has
   essentially zero platform #ifdefs; a platform = one directory + skeleton + toolchain
   image. tg5050 proves the clone-and-repoint model.
4. **The old MinUI rg35xxplus port is recoverable from git** (`git show 8cd78866:...`)
   and documents every H700 hardware path — buttons, brightness ioctl, rumble, lid,
   HDMI, boot hijack — most re-verified live on 2026 firmware during this planning.
   It is a *hardware-paths reference only*: its software architecture (32-bit,
   SDL_Renderer, pre-shader-era APIs) is obsolete; **tg5040 is the golden platform**
   and the code clone base.
5. **The boot hijack still exists on current firmware** (verified in the stock
   launcher scripts on-device): drop `dmenu.bin` on the FAT ROMs partition → stock
   runs it instead of its frontend. Reversible by deleting one file.

## Key decisions (rationale in the linked docs)

| Decision | Choice | Doc |
|---|---|---|
| Platform name | `h700` (one platform, `DEVICE` env selects rg40xx/rg34xx/rg28xx/cube — the tg5040 `is_brick` pattern) | 03 |
| Arch | aarch64 / cortex-a53, 64-bit (matches stock OS & tg5040 flags) | 01 |
| Toolchain | **Reuse the tg5040 docker image** (empirically proven: tg5040-built displaycal.elf runs on stockmod + Knulli); custom SDL2 built in-tree (`workspace/h700/other/`, old-port pattern); dedicated image deferred | 01 |
| Video | Bundled custom SDL2 (JohnnyonFlame/SDL-malifbdev-rot, rebuilt 64-bit) + Mali-G31 blob GLES 3.2 → NextUI's generic_video/shader pipeline unchanged | 04 |
| Rotation (28xx) | Try SDL-level rot first; durable plan = small rotation hook in generic_video present pass | 04 |
| Sleep | `echo mem` (verified) + tg5040-style suspend wrapper (wifi bounce on resume required — observed); power-button wake; RTC wake unavailable | 06 |
| Lid (34XXSP) | `axp2202-battery/hallkey`, old-port semantics in current `PLAT_initLid/lidChanged` API | 06 |
| WiFi | NextUI-owned wpa_supplicant (generic_wifi.c unchanged); stop NetworkManager at launch | 07 |
| BT | System BlueZ 5.64 + ship bluealsa (no btmanager pakz needed) | 07 |
| Rumble | `axp2202-battery/moto` (on/off) | 03 |
| LEDs | `MAX_LIGHTS 0`; `work_led`/`workled_sleep` for power-LED sleep signaling only | 03 |
| Brightness / displaycal | Same `/dev/disp` ioctls as tg5040 (0x102 brightness, 0x10b-d gamma LUT) — expected 1:1 | 04 |
| Install location | NextUI entirely on TF2 (`/mnt/sdcard`); stock TF1 card untouched except one drag-dropped `dmenu.bin` on its FAT ROMs partition (the verified stock-boot hijack; stock OS partitions never written). No TF1-only mode. | 02 |

## Top risks (each has a fallback documented)

1. ~~Mali blob 32-bit-only / wrong winsys~~ **RESOLVED ✔** (verified on device):
   blob is 64-bit aarch64, OpenGL ES 3.2 (r20p0, matching kernel kbase), fbdev EGL
   winsys — and the stock OS's own SDL2 is 64-bit built on the `mali` video driver.
   The planned stack is exactly what the device already runs (00/04).
2. **NextUI UI regressions at 480p** — no 480-line platform has existed for ~2 years;
   layout/pill/font audit budgeted (04).
3. **RG28XX rotation through the GL path** — the malifbdev-rot patch may only rotate
   the non-GL blitter; the generic_video rotation hook is the designed fallback (04).
4. **stockmod boot precedence** (`muos1.ini` beats our hijack) — documentation +
   installer detection (02).
5. **Sleep-resume WiFi/panel quirks** — resume-side re-init is planned work, not an
   afterthought (06); 20-cycle + overnight-drain acceptance tests (08).

## Timeline

~6–8 weeks single-dev to a 40XXV + 34XXSP beta; RG28XX ≈ +1 week after. Phase table
with per-phase exit criteria in [08-testing-and-rollout.md](08-testing-and-rollout.md).
Start with the Phase-0 spike list — every architectural bet above gets proven or
re-planned within the first week, before any large code investment.
