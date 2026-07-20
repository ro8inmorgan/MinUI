# NextUI → Allwinner H700 (Anbernic RG XX) Port

**Goal:** NextUI on the Anbernic RG XX line (Allwinner H700) with feature parity with
the TrimUI Brick (tg5040) — displaycal, WiFi/BT, first-class sleep — installed *on top
of the stock Anbernic OS*, no reflash, fully reversible.

**Status (2026-07-20): ready for beta1 with documented known issues; RC gates listed
in [09](09-roadmap.md).**
RG40XXV has broad hardware coverage. RG34XXSP now passes the complete 720×480 UI
sweep, box art, Files, Input, Recently Played, game switcher, RetroAchievements,
displaycal persistence, manual governor changes, lid sleep/wake, Bootlogo, and
GB/GBC/GBA/FC/SFC (plus A2600/MGBA/SMS and MD/PS). RG28XX is now user-tested: driver-level rotation validated,
minarch Aspect/Fullscreen scaling fixed (a double-rotation bug), and Bootlogo
apply + rotated previews working.
The apparent RG34XXSP PS1 launch regression was a stale/corrupted core artifact and a
clean rebuild fixed it. MD Auto CPU is fixed. FBNeo plays OK with BIOS; missing-BIOS
hard-lock is a **beta1 known issue** and an **RC must-fix**. Closed-lid POWER and
charging→light-sleep-only are accepted policies. Screenshots and multi-panel UI
scaling pass. Battery % vs stock passes (icon-vs-Battery-pak desync after long sleep
is follow-up). Overnight drain partial (~7% / 8.5 h; voltage/stock re-test = RC
polish). Clean uninstall removed from the test matrix (delete `dmenu.bin` is
design-guaranteed). SIGUSR1 and dirty-card recovery are **RC polish** (untested OK
for beta1). Panel-matched overlay assets not shipped for H700 sizes. Full RGcubexx
matrix remains external. BT audio used stock BlueALSA with an earlier AirPods pass;
lifecycle revalidation blocked by device-wide BT scan failure. Pak Store and OTA
out of scope. Full status: [08](08-testing-status.md), [09](09-roadmap.md),
[10](10-core-game-matrix.md).

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
| [05-audio.md](05-audio.md) | ALSA path, the dlopen'd-libasound bug story, volume/mute quirks, BT-audio lifecycle |
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
| Video | generic_video GLES pipeline on Mali blob | ✅ core pipeline works; multi-panel UI scaling + box art pass; panel-matched overlay assets not shipped |
| **Input** | SDL joystick route | **Deviation:** raw evdev primary (SDL js enumeration unreliable on stock image); SDL kept for BT pads (03) |
| **Audio linkage** | SDK libasound, bundled | **Deviation:** dlopen'd device libasound (`--enable-alsa-shared`) after a symbol-versioning bug caused glitchy audio (05) |
| **BT audio** | build + ship bluealsa | **Deviation:** use stock BlueALSA 4.2.0, ALSA plugins, SBC, and BlueZ without bundling replacements; AirPods 4 ANC SBC playback passed on RG40XXV after H700-only ALSA-option omission + native-volume init; auto reconnect untested; revalidation blocked by device-wide BT scan/pair failure on stock and BaseOS (05/07) |
| Sleep | `echo mem` + tg5040-style wrapper | ✅ repeated sleep/wake, in-game resume, and power-off auto-resume pass; charging→light-sleep-only accepted policy (06) |
| Lid | hallkey → PLAT lid API | ✅ close/open works; POWER-while-closed accepted policy (light sleep only; deep re-sleeps if lid closed) |
| WiFi | NextUI-owned wpa_supplicant | ✅ as planned; creds on SD, dhclient + wpa_action renew (07) |
| Rumble / LEDs | moto sysfs; MAX_LIGHTS 0 | ✅ as planned; rumble tested-good |
| Brightness / displaycal | same disp ioctls as tg5040 | ✅ 1:1 as predicted, tested-good incl. sleep survival |
| Install | TF1 gets one file; NextUI on TF2; no TF1-only mode | ✅ as planned; TF1 writes mountpoint+cmp guarded |
| **Splash** | per-panel raw-BMP `dd` assets | **Deviation:** `fbsplash` text renderer — no per-resolution assets needed (02) |
| RG28XX rotation | SDL-level first, GL hook fallback | ✅ SDL/driver level alone is correct; the GL hook double-rotated and broke minarch scaling — removed after hardware testing (04) |
| HDMI | stretch goal | ✅ RG40XXV menu/game output, audio routing, and in-game hotplug pass in both directions; boot-with-cable, other models, and unusual EDIDs remain follow-ups (04) |

## Top open risks

1. **FBNeo missing-BIOS hard-lock** — major; **RC must-fix**. beta1 ships as known
   issue (happy path with BIOS OK). MD Auto CPU fixed. Closed-lid POWER and
   charging→light-sleep accepted policy.
2. **RC polish (untested OK for beta1)** — SIGUSR1 shutdown, dirty-SD recovery;
   overnight drain voltage/stock re-test. Clean uninstall not a test row (delete
   `dmenu.bin`).
3. **Partially tested device matrix** — RG40XXV, RG34XXSP, and RG28XX are working
   (RG28XX rotation/scaling validated 2026-07-12); RGcubexx has community UI-scaling
   validation, with full matrix still external.
4. **Stock-OS coupling** — model detection and the `dmenu.bin` hijack read Anbernic
   binaries/scripts; a firmware update can move them. Stock **MU style** themes
   bypass the hijack entirely (install requires **old style** — 02/08).
5. **Shared-UI capability regressions** — the Input tester is device-aware, dead
   display controls are hidden/verified, and Fn-switch settings are now gated off.
   Keep the systematic capability sweep as shared UI continues to evolve.
   (Formerly listed here: the alpha-blit unknown — resolved; it was the missing
   64-bit libpng. The branch includes main's alpha blending; game switcher/overlay
   rendering is clean; screenshots and multi-panel UI scaling pass
   (incl. community 720×720); panel-matched overlay assets not shipped, 04.)
