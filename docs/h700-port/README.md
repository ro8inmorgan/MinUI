# NextUI → Allwinner H700 (Anbernic RG XX) Port

**Goal:** NextUI on the Anbernic RG XX line (Allwinner H700) with feature parity with
the TrimUI Brick (tg5040) — displaycal, WiFi/BT, first-class sleep — installed *on top
of the stock Anbernic OS*, no reflash, fully reversible.

**Lifecycle:** preparing the Beta candidate. See [09](09-roadmap.md) for stage gates
and [08](08-testing-status.md) for current evidence.

This directory is the H700 maintainer knowledge base. Documents 00–07 record durable
facts, architecture, decisions, rationale, failure analysis, diagnostics, and lessons
for future platform ports. They may say how a fact was established, but they do not
own current release status. Documents 08–10 are the operational ledgers for tests,
release planning, and per-core coverage.

## Documents

| Doc | Role | Contents |
|---|---|---|
| [00-device-facts.md](00-device-facts.md) | Knowledge base | Probed hardware/OS facts, sysfs/ioctl/evdev reference, stock boot chain, known unknowns |
| [01-toolchain-and-build.md](01-toolchain-and-build.md) | Knowledge base | dedicated h700-toolchain image, in-tree SDL2, dependency policy, and cross-toolchain lessons |
| [02-boot-and-installer.md](02-boot-and-installer.md) | Knowledge base | `dmenu.bin` hijack, storage/write policy, boot shim, installer, and launcher responsibilities |
| [03-platform-layer.md](03-platform-layer.md) | Knowledge base | Platform API map, device detection, evdev/SDL input split, libmsettings, keymon, and cores |
| [04-video-display.md](04-video-display.md) | Knowledge base | Mali/fbdev architecture, geometry, rotation and HDMI decision records, DisplayCal, graphics incidents |
| [05-audio.md](05-audio.md) | Knowledge base | ALSA architecture, symbol-versioning incident, routing, Bluetooth audio, and sample-rate constraints |
| [06-power-sleep-battery.md](06-power-sleep-battery.md) | Knowledge base | Sleep architecture/postmortem, lid semantics, charging behavior, battery facts, and governors |
| [07-wifi-bluetooth.md](07-wifi-bluetooth.md) | Knowledge base | Stock components, NextUI ownership boundaries, lifecycle scripts, and diagnostics |
| [08-testing-status.md](08-testing-status.md) | Test ledger | Stable test IDs, current results, evidence devices, and model-specific details |
| [09-roadmap.md](09-roadmap.md) | Release policy | Alpha 1/2/3, Beta, RC, and Release gates; deferrals, future work, enhancements, and stretch goals |
| [10-core-game-matrix.md](10-core-game-matrix.md) | Test ledger | Per-system/core/game evidence and emulator defects underlying the GAME test rows |

## Architecture in one paragraph

One `h700` platform serves rg40xx/rg34xx/rg28xx/cube (`DEVICE` env, detected at boot
from the stock dmenu.bin binary). It builds inside the **h700-toolchain image** (same
aarch64/A53 target and TG5040-derived SDK sysroot as the historical tg5040 reuse,
forward-compatible glibc) with one in-tree extra: a pinned custom SDL2
(`JohnnyonFlame/SDL-malifbdev-rot`) targeting the Mali blob's fbdev EGL winsys.
NextUI's shared `generic_video.c`/`generic_wifi.c`/`generic_bt.c` run unchanged. Boot
is hijacked by dropping one `dmenu.bin` file on the stock card's FAT partition; NextUI
itself lives entirely on TF2. The stock Ubuntu userland is used aggressively
(systemd, timedatectl, dhclient, firmware-provided BlueZ, stock unzip) rather than
bundled around.

## Key decisions — outcome register

| Decision | Planned | Shipped / outcome |
|---|---|---|
| Platform | one `h700`, `DEVICE` env per device | ✅ as planned |
| Arch / toolchain | 64-bit; dedicated `h700-toolchain` (TG5040 SDK recipe) | ✅ dedicated image; sysroot-mismatch pitfalls catalogued in 01 |
| SDL2 | in-tree malifbdev-rot build | ✅ as planned, pinned + config-asserted |
| Video | generic_video GLES pipeline on Mali blob | ✅ core pipeline works; multi-panel UI scaling + box art pass; panel-matched overlay assets not shipped |
| **Input** | SDL joystick route | **Deviation:** raw evdev remains primary for the built-in pad; the SDL classification fix makes enumeration work, but the built-in SDL ordering differs from external-pad `JOY_*` mappings and would duplicate/mis-map events. SDL is retained for external pads (03) |
| **Audio linkage** | SDK libasound, bundled | **Deviation:** dlopen'd device libasound (`--enable-alsa-shared`) after a symbol-versioning bug caused glitchy audio (05) |
| **BT audio** | build + ship bluealsa | **Deviation:** use the stock BlueALSA/ALSA/SBC/BlueZ stack, with an H700-only ALSA option adjustment, initial mixer volume, and post-connect mixer update (05/07) |
| Sleep | `echo mem` + tg5040-style wrapper | ✅ shared power architecture retained; target-specific service and ALSA sequencing documented in 06 |
| Lid | hallkey → PLAT lid API | ✅ sysfs-polled lid integrated with the shared sleep request API (06) |
| WiFi | NextUI-owned wpa_supplicant | ✅ as planned; creds on SD, dhclient + wpa_action renew (07) |
| Rumble / LEDs | moto sysfs; MAX_LIGHTS 0 | ✅ as planned; rumble tested-good |
| Brightness / displaycal | same disp ioctls as tg5040 | ✅ 1:1 as predicted, tested-good incl. sleep survival |
| Install | TF1 gets one file; NextUI on TF2; no TF1-only mode | ✅ as planned; TF1 writes mountpoint+cmp guarded |
| **Splash** | per-panel raw-BMP `dd` assets | **Deviation:** `fbsplash` text renderer — no per-resolution assets needed (02) |
| RG28XX rotation | SDL-level first, GL hook fallback | ✅ SDL/driver level alone is correct; the GL hook double-rotated and broke minarch scaling — removed after hardware testing (04) |
| HDMI | stretch goal | ✅ disp2/fbdev hotplug with layer-geometry repair and ALSA routing (04) |

## Maintenance model

- Update 00 when hardware, firmware, stock-OS paths, or empirical facts change.
- Update 01–07 when implementation, rationale, diagnostics, or a subsystem lesson
  changes. Keep incident histories when they explain a non-obvious constraint.
- Record test evidence once in 08 and per-core evidence once in 10.
- Record stage gates, accepted risk, deferrals, enhancements, and future work only in
  09. Subsystem documents should link there instead of restating release policy.
- Re-probe stock integration after Anbernic firmware changes and revisit the relevant
  knowledge-base chapter before treating a new failure as a NextUI regression.
