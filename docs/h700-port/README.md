# NextUI → Allwinner H700 (Anbernic RG XX) Port

**Goal:** NextUI on the Anbernic RG XX line (Allwinner H700) with feature parity with
the TrimUI Brick (tg5040) — displaycal, WiFi/BT, first-class sleep — installed *on top
of the stock Anbernic OS*, no reflash, fully reversible.

**Status (2026-07-13): ready for an alpha release, with documented RG34XXSP issues.**
RG40XXV has broad hardware coverage. RG34XXSP now passes the complete 720×480 UI
sweep, box art, Files, Input, Recently Played, game switcher, RetroAchievements,
displaycal persistence, manual governor changes, lid sleep/wake, Bootlogo, and
GB/GBC/GBA/FC/SFC. RG28XX is now user-tested: driver-level rotation validated,
minarch Aspect/Fullscreen scaling fixed (a double-rotation bug), and Bootlogo
apply + rotated previews working.
Open RG34XXSP issues are PS1 launch crashes, an FBNeo missing-BIOS lockup, MD Auto CPU
scaling sensitivity, POWER waking through a closed
lid, and charging remaining in light sleep by current shared policy. Screenshots,
resolution-specific overlays, battery accuracy/overnight drain, recovery paths, and
RGcubexx hardware coverage remain untested. The cube is intentionally an
external alpha-validation target. BT audio, Pak Store, and OTA update are explicitly
outside this alpha scope. Full status is in [08](08-testing-status.md), priorities in
[09](09-roadmap.md), and emulator coverage in [10](10-core-game-matrix.md).

These docs began as the implementation plan and were restructured after the
implementation landed (branch `h700`) into reference documentation:
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
| [06-power-sleep-battery.md](06-power-sleep-battery.md) | Sleep design, lid/charging edge cases, battery, governors |
| [07-wifi-bluetooth.md](07-wifi-bluetooth.md) | NextUI-owned wpa_supplicant, DHCP/creds handling, BT status |
| [08-testing-status.md](08-testing-status.md) | Validation matrix with real results, regression guardrails, shared-code touch list |
| [09-roadmap.md](09-roadmap.md) | Prioritized path from alpha candidate to a 9.5/10 port |
| [10-core-game-matrix.md](10-core-game-matrix.md) | Per-system/core/game coverage and open emulator defects |

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
| Video | generic_video GLES pipeline on Mali blob | ✅ core pipeline works; 720×480 UI and box art pass, resolution-specific overlays untested |
| **Input** | SDL joystick route | **Deviation:** raw evdev primary (SDL js enumeration unreliable on stock image); SDL kept for BT pads (03) |
| **Audio linkage** | SDK libasound, bundled | **Deviation:** dlopen'd device libasound (`--enable-alsa-shared`) after a symbol-versioning bug caused glitchy audio (05) |
| **BT audio** | build + ship bluealsa | **Deviation:** gated off this alpha (`NO_BT_AUDIO`); re-enable path documented (05/07) |
| Sleep | `echo mem` + tg5040-style wrapper | ⚠ repeated sleep/wake, in-game resume, and power-off auto-resume pass; charging intentionally stays in light sleep (06) |
| Lid | hallkey → PLAT lid API | ⚠ close/open works, but POWER can wake RG34XXSP while the lid remains closed |
| WiFi | NextUI-owned wpa_supplicant | ✅ as planned; creds on SD, dhclient + wpa_action renew (07) |
| Rumble / LEDs | moto sysfs; MAX_LIGHTS 0 | ✅ as planned; rumble tested-good |
| Brightness / displaycal | same disp ioctls as tg5040 | ✅ 1:1 as predicted, tested-good incl. sleep survival |
| Install | TF1 gets one file; NextUI on TF2; no TF1-only mode | ✅ as planned; TF1 writes mountpoint+cmp guarded |
| **Splash** | per-panel raw-BMP `dd` assets | **Deviation:** `fbsplash` text renderer — no per-resolution assets needed (02) |
| RG28XX rotation | SDL-level first, GL hook fallback | ✅ SDL/driver level alone is correct; the GL hook double-rotated and broke minarch scaling — removed after hardware testing (04) |
| HDMI | stretch goal | Detection wired; `SetHDMI()` no-op — still a stretch goal (04) |

## Top open risks

1. **RG34XXSP runtime issues** — PS1 launch crashes, FBNeo can trap MinArch after a
   missing-BIOS failure, MD Auto CPU scaling is render-setting-sensitive, and POWER
   can wake through the closed lid. (Bootlogo previews at 720×480: fixed 2026-07-13.)
2. **Partially tested robustness matrix** — clean uninstall, dirty-SD recovery,
   SIGUSR1 shutdown, battery accuracy, screenshots, and overnight drain need runs.
3. **Partially tested device matrix** — RG40XXV, RG34XXSP, and RG28XX are working
   (RG28XX rotation/scaling validated 2026-07-12); RGcubexx is an external alpha
   target.
4. **Stock-OS coupling** — model detection, hijack point, and muOS interplay all read
   Anbernic's binaries/scripts; a firmware update can move them (guardrails in 08).
5. **Shared-UI capability regressions** — the Input tester is device-aware, dead
   display controls are hidden/verified, and Fn-switch settings are now gated off.
   Keep the systematic capability sweep as shared UI continues to evolve.
   (Formerly listed here: the alpha-blit unknown — resolved; it was the missing
   64-bit libpng. The branch includes main's alpha blending; game switcher/overlay
   rendering is clean, with screenshots and resolution-specific overlays still to
   check, 04.)
