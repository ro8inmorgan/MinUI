# NextUI → Allwinner H700 (Anbernic RG XX) Port

**Goal:** NextUI on the Anbernic RG XX line (Allwinner H700) with feature parity with
the TrimUI Brick (tg5040) — displaycal, WiFi/BT, first-class sleep — installed *on top
of the stock Anbernic OS*, no reflash, fully reversible.

**Status (2026-07-09): working beta on RG40XXV and RG34XXSP.** Boot, video (custom
Mali SDL2 + full GLES shader pipeline), audio, input, games at full speed,
brightness/colortemp/displaycal, WiFi, and rumble are all tested-good on RG40XXV
hardware. RG34XXSP has also been user-tested and behaves the same as RG40XXV, except
lid wake/sleep does not work yet (the screen stays on). Known open items: sleep wake is
unreliable (the #1 defect), BT audio is gated off, RG28XX/cube are wired but untested.
Full status in [08](08-testing-status.md), path to done in [09](09-roadmap.md).

These docs began as the implementation plan and were restructured after the
implementation landed (branch `h700`, 17 commits) into reference documentation:
verified device facts, the architecture as shipped, deviations from the plan and why,
and the lessons that transfer to future platform ports.

## Documents

| Doc | Contents |
|---|---|
| [00-device-facts.md](00-device-facts.md) | Ground truth: probed hardware facts, sysfs/ioctl/evdev reference, boot chain, quirks |
| [01-toolchain-and-build.md](01-toolchain-and-build.md) | tg5040-image reuse, in-tree SDL2, **toolchain-reuse pitfalls (read before porting anything else)** |
| [02-boot-and-installer.md](02-boot-and-installer.md) | dmenu.bin hijack, boot shim, installer, SD layout, uninstall |
| [03-platform-layer.md](03-platform-layer.md) | `workspace/h700/`: platform.c/h, input (evdev-primary), libmsettings, keymon, cores |
| [04-video-display.md](04-video-display.md) | SDL2+Mali/GLES stack, geometry, rotation, HDMI, displaycal, the alpha-blit question |
| [05-audio.md](05-audio.md) | ALSA path, the dlopen'd-libasound bug story, volume/mute quirks, BT-audio gating |
| [06-power-sleep-battery.md](06-power-sleep-battery.md) | Sleep design, **the wake-reliability issue**, lid, battery, governors |
| [07-wifi-bluetooth.md](07-wifi-bluetooth.md) | NextUI-owned wpa_supplicant, DHCP/creds handling, BT status |
| [08-testing-status.md](08-testing-status.md) | Validation matrix with real results, regression guardrails, shared-code touch list |
| [09-roadmap.md](09-roadmap.md) | Prioritized path from beta to a 9.5/10 port |

## Architecture in one paragraph

One `h700` platform serves rg40xx/rg34xx/rg28xx/cube (`DEVICE` env, detected at boot
from the stock dmenu.bin binary). It builds inside the **tg5040 toolchain image** (same
aarch64/A53 target, forward-compatible glibc) with one in-tree extra: a pinned custom
SDL2 (`JohnnyonFlame/SDL-malifbdev-rot`) targeting the Mali blob's fbdev EGL winsys.
NextUI's shared `generic_video.c`/`generic_wifi.c`/`generic_bt.c` run unchanged. Boot
is hijacked by dropping one `dmenu.bin` file on the stock card's FAT partition; NextUI
itself lives entirely on TF2. The stock Ubuntu userland is used aggressively
(systemd, timedatectl, dhclient, BlueZ 5.64, stock unzip) rather than bundled around.

## Key decisions — outcome register

| Decision | Planned | Shipped / outcome |
|---|---|---|
| Platform | one `h700`, `DEVICE` env per device | ✅ as planned |
| Arch / toolchain | 64-bit, reuse tg5040 image | ✅ as planned — with real costs; pitfalls catalogued in 01 |
| SDL2 | in-tree malifbdev-rot build | ✅ as planned, pinned + config-asserted |
| Video | generic_video GLES pipeline on Mali blob | ✅ worked 1:1; shaders/overlays/effects tested-good |
| **Input** | SDL joystick route | **Deviation:** raw evdev primary (SDL js enumeration unreliable on stock image); SDL kept for BT pads (03) |
| **Audio linkage** | SDK libasound, bundled | **Deviation:** dlopen'd device libasound (`--enable-alsa-shared`) after a symbol-versioning bug caused glitchy audio (05) |
| **BT audio** | build + ship bluealsa | **Deviation:** gated off this beta (`NO_BT_AUDIO`); re-enable path documented (05/07) |
| Sleep | `echo mem` + tg5040-style wrapper | ✅ implemented incl. resume restore — ⚠️ wake unreliable, open (06) |
| Lid | hallkey → PLAT lid API | ⚠️ wired, but RG34XXSP lid wake/sleep does not work yet (screen stays on) |
| WiFi | NextUI-owned wpa_supplicant | ✅ as planned; creds on SD, dhclient + wpa_action renew (07) |
| Rumble / LEDs | moto sysfs; MAX_LIGHTS 0 | ✅ as planned; rumble tested-good |
| Brightness / displaycal | same disp ioctls as tg5040 | ✅ 1:1 as predicted, tested-good incl. sleep survival |
| Install | TF1 gets one file; NextUI on TF2; no TF1-only mode | ✅ as planned; TF1 writes mountpoint+cmp guarded |
| **Splash** | per-panel raw-BMP `dd` assets | **Deviation:** `fbsplash` text renderer — no per-resolution assets needed (02) |
| RG28XX rotation | SDL-level first, GL hook fallback | Both layers plumbed; unvalidated (04) |
| HDMI | stretch goal | Detection wired; `SetHDMI()` no-op — still a stretch goal (04) |

## Top open risks

1. **Sleep/wake hangs** — headline feature not yet trustworthy ([06](06-power-sleep-battery.md), roadmap P0).
2. **Partially tested device matrix** — RG34XXSP broadly works, but lid handling is
   broken; RG28XX/cube code paths have never met hardware.
3. **Stock-OS coupling** — model detection, hijack point, and muOS interplay all read
   Anbernic's binaries/scripts; a firmware update can move them (guardrails in 08).
4. **Brick-era assumptions in shared UI** — Input tester shows Brick's layout, an
   Fn-switch setting exists for hardware RG XX doesn't have, and dead enhance
   display controls are exposed; needs a capability-flag sweep (09-roadmap #8).
   (Formerly listed here: the alpha-blit unknown — resolved; it was the missing
   64-bit libpng. The branch is now rebased onto main incl. alpha blending; only
   the on-device rendering re-check remains, 04.)
