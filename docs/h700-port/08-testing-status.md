# 08 — Testing Status

This document is the canonical H700 test report. It records what was tested, the
result, and the device that supplied the evidence. It does not decide whether a test
blocks Alpha, Beta, RC, or Release; those decisions live in
[09-roadmap.md](09-roadmap.md). Detailed per-core coverage lives in
[10-core-game-matrix.md](10-core-game-matrix.md).

Results recorded through 2026-07-20.

## Status and evidence rules

| Status | Meaning |
|---|---|
| ✅ Pass | The stated test completed successfully |
| ⚠️ Partial | Useful behavior passed, but the row has a known issue or incomplete coverage |
| ⬜ Not run | No completed result is recorded yet |
| ➖ Not applicable | The behavior is unsupported or guaranteed by design rather than a runtime test |
| ❌ Fail | The stated test failed |

The H700 devices share one platform implementation, SoC, stock-OS integration, and
core runtime. A successful shared-runtime test on one representative H700 device
counts for the family. Repeat it on another model only when panel geometry/rotation,
lid, controls, connectors, firmware, or model-specific code differs, or when evidence
suggests a model-specific regression. A named device is the evidence source, not a
limit on support.

## Test fleet

| Device | Evidence | Hardware coverage |
|---|---|---|
| RG40XXV | Probed live | 640×480, one stick, HDMI, Bluetooth audio; family baseline |
| RG34XXSP | User-tested | 720×480, dual sticks, clamshell lid |
| RG28XX | User-tested | 480×640 portrait panel and rotation |
| RGcubexx | Community-tested | 720×720 general UI scaling only |
| TrimUI Brick TG5040 | Probed live | Shared-code regression reference |

## Build and binary compatibility

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| BUILD-01 | CI-style H700 staging build (`make setup`, then `make h700`) | ✅ | CI/release build | H700 binaries, tools, cores, libraries, boot shim, and installer assets stage successfully |
| BUILD-02 | Full tg5040 regression build after shared-code changes | ⬜ | Candidate-specific | Must be recorded for the actual candidate; single-pak builds are insufficient |
| BUILD-03 | Jammy `settings.elf` runtime linkage check | ✅ | Automated container check | H700 library path resolves and required gio/glib linkage is present |

## Boot, install, and launcher

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| BOOT-01 | Cold boot to NextUI and 10+ reboot soak | ✅ | RG40XXV | Stable boot with reasonable startup time |
| BOOT-02 | Stock-card hijack with the required **old style** theme | ✅ | RG40XXV/RG34XXSP | `/mnt/mmc/dmenu.bin` launches NextUI from TF2 |
| BOOT-03 | Repeated launcher crash limit | ⬜ | Code present | The launcher implements a five-consecutive-crash cutoff followed by poweroff, but it has not been exercised end to end |
| BOOT-04 | Remove `dmenu.bin` to restore stock boot | ➖ | Design inspection | Stock `dmenu_ln` cannot launch NextUI when the single hijack file is absent; no separate uninstall workflow exists |
| BOOT-05 | Stock **MU style** theme behavior | ✅ | On-device observation | MU style bypasses `/mnt/mmc/dmenu.bin` and boots the stock/MU frontend; NextUI cannot display an in-shim warning |

## Display and user interface

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| DISP-01 | Launcher, settings, keyboard, and in-game UI rendering | ✅ | RG40XXV, RG34XXSP, RG28XX | Clean rendering at 640×480, 720×480, and rotated 480×640 |
| DISP-02 | Brightness 0–10 and color-temperature behavior/persistence | ✅ | RG40XXV | Controls act and persist; fresh default brightness is 4 |
| DISP-03 | DisplayCal RGB gains, persistence, sleep, and game launch | ✅ | RG40XXV, RG34XXSP | Gains act and persist across reboot, sleep, and game launch |
| DISP-04 | Dead enhance controls and Fn-switch settings hidden | ✅ | RG40XXV/code inspection | Contrast/saturation/exposure and the nonexistent RG XX Fn switch are capability-gated |
| DISP-05 | Shaders, overlay pipeline, and effects on Mali-G31 | ✅ | RG40XXV | All shipped GLSL paths tested; this tests the pipeline, not panel-sized overlay content |
| DISP-06 | Box art, game switcher, and alpha-blended assets | ✅ | RG34XXSP | Clean at 720×480 after the 64-bit libpng fix |
| DISP-07 | Screenshots | ✅ | RG34XXSP | Capture succeeds at 720×480 |
| DISP-08 | HDMI video hotplug and scaling | ✅ | RG40XXV + TV | Menu and game output, plug/unplug in both directions, repeated cycles, autosave/resume, and 1280×720 framebuffer scaling pass |

## Input and controllers

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| INPUT-01 | Local controls in launcher, games, and menus | ✅ | RG40XXV, RG34XXSP, RG28XX | Raw evdev input path works across tested models |
| INPUT-02 | MENU short tap versus hold | ✅ | RG34XXSP | Short tap opens shortcuts; hold changes brightness despite the firmware compound event |
| INPUT-03 | Per-device stick layout, analog movement, and L3/R3 | ✅ | RG40XXV, RG34XXSP | The Input pak correctly visualizes the RG40XXV left stick and the RG34XXSP left/right sticks, including movement and their L3/R3 clicks |
| INPUT-04 | Rumble | ✅ | RG40XXV | `moto` on/off path works |
| INPUT-05 | Bluetooth controller discovery, pairing, and attachment | ✅ | RG40XXV + DualSense | Discovers, pairs, trusts, connects, and exposes SDL joystick plus evdev; five restart cycles produced no duplicate daemon/attach processes |
| INPUT-06 | Bluetooth controller semantic mapping | ⚠️ | DualSense/shared tg50x0 path | Attachment passes, but raw button indices vary by controller and are not normalized across platforms |

## Audio

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| AUDIO-01 | Internal volume UI, mute, and levels from 0–100% | ✅ | RG40XXV, RG34XXSP | Audible levels and reversed `digital volume` scale behave correctly |
| AUDIO-02 | Game audio and underruns | ✅ | RG40XXV, tested RG34XXSP games | GBA/SNES and broader gameplay have clean audio with no observed underruns |
| AUDIO-03 | Headphone insertion and speaker routing | ✅ | RG34XXSP | Hardware auto-mutes the speaker and routes to headphones; NextUI does not expose a separate HP icon/volume |
| AUDIO-04 | Bluetooth A2DP pairing and playback | ✅ | RG40XXV + AirPods 4 ANC | Pairing is remembered; manual Connect routes audible SBC audio; returning AirPods to the case restores the internal speaker. Nearby iPhone/Mac devices may win auto-connect |
| AUDIO-05 | HDMI audio routing and restoration | ✅ | RG40XXV + TV | HDMI route is audible; unplug restores the panel/internal route |

## Network and online features

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| NET-01 | WiFi scan, connect, and forget | ✅ | RG40XXV | NextUI-owned wpa_supplicant path works |
| NET-02 | WiFi after sleep and NTP synchronization | ✅ | RG40XXV | Network recovers after sleep and system time synchronizes |
| NET-03 | RetroAchievements login and unlock | ✅ | RG34XXSP | Credential entry through the on-screen keyboard and achievement unlock pass |
| NET-04 | Pak Store install | ➖ | Scope | Not exercised for the H700 port |
| NET-05 | OTA update flow | ➖ | Scope | Not exercised for the H700 port |

## Power, sleep, and battery

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| POWER-01 | Manual/automatic deep sleep and wake | ✅ | RG40XXV, RG34XXSP | Repeated sleep/wake, in-game resume, and wake without the former ALSA delay pass |
| POWER-02 | Fast resume and power-off auto-resume | ✅ | RG40XXV, RG34XXSP | Resume returns to the running game as designed |
| POWER-03 | RG34XXSP lid close/open | ✅ | RG34XXSP | Close sleeps and open wakes. POWER may wake light sleep with lid closed; deep sleep re-sleeps while the lid remains closed |
| POWER-04 | Charging indication and sleep while charging | ✅ | RG34XXSP | Indicator works; shared product behavior permits light/screen-off sleep and suppresses deep sleep while charging |
| POWER-05 | Battery percentage versus stock | ✅ | RG34XXSP | Fresh-boot percentage matches stock; after long sleep the top-bar icon and Battery pak briefly disagreed with each other |
| POWER-06 | Overnight suspend drain versus stock | ⚠️ | RG34XXSP | First point: about 7% over 8.5 hours. No voltage-based stock comparison is recorded |

## Games and performance

See [10-core-game-matrix.md](10-core-game-matrix.md) for the game-by-game ledger.

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| GAME-01 | Browse, launch, play, menu, save/load, and clean exit | ✅ | RG40XXV, RG34XXSP | Primary system pak/core flow works end to end |
| GAME-02 | GB/GBC/GBA/FC/SFC gameplay smoke | ✅ | RG34XXSP | Tested games launch and play correctly |
| GAME-03 | MD/PicoDrive launch, gameplay, and Auto CPU | ✅ | RG34XXSP | Former slowdown near 480 MHz is fixed |
| GAME-04 | PS1/PCSX-ReARMed launch and performance | ✅ | RG40XXV, RG34XXSP | Clean core rebuild fixed the stale/corrupted packaged artifact |
| GAME-05 | FBNeo happy path and missing-BIOS error path | ⚠️ | RG34XXSP | With required BIOS, launch/play pass. Without BIOS, the upstream core stops polling frontend input and MinArch cannot open menu/exit |
| GAME-06 | Remaining EXTRAS core smoke | ⚠️ | RG34XXSP | A2600, MGBA, and SMS pass; the remaining long-tail systems are not fully covered |
| PERF-01 | GBA/SNES full speed with vsync | ✅ | RG40XXV, tested RG34XXSP games | Full speed with no observed audio underruns |
| PERF-02 | Frame pacing, tearing, and input latency | ✅ | RG40XXV | No visible issue during gameplay; result is observational, not instrumented |
| PERF-03 | Manual CPU governor changes during play | ✅ | RG34XXSP | Powersave/performance changes do not stutter audio |
| PERF-04 | Automatic governor load/frequency selection | ✅ | RG34XXSP | MD Auto CPU regression is resolved |

## Applications and device-specific hardware

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| APP-01 | Files/NextCommander input, scaling, and first frame | ✅ | RG40XXV, RG34XXSP | Works at PPU 2 |
| APP-02 | Recently Played and game switcher | ✅ | RG40XXV, RG34XXSP | Both flows work; rendered assets are clean |
| DEV-01 | RG40XXV 640×480/single-stick baseline | ✅ | RG40XXV | UI, input, game runtime, HDMI, and Bluetooth paths pass |
| DEV-02 | RG34XXSP 720×480/lid/dual-stick delta | ✅ | RG34XXSP | UI scaling, lid, controls, apps, and games pass |
| DEV-03 | RG28XX 480×640 rotation delta | ✅ | RG28XX | UI, game Aspect/Fullscreen scaling, bootlogo apply, and rotated preview pass |
| DEV-04 | RGcubexx 720×720 delta | ⚠️ | Community report | General UI scaling passes; no full local functional/core walk exists |
| DEV-05 | Bootlogo 640×480 apply/backup | ✅ | RG40XXV | Carousel, apply, backup, and original entry pass; restore uses the same apply path |
| DEV-06 | Bootlogo 720×480 preview/apply/backup/restore | ✅ | RG34XXSP | All 23 presets render; first backup and restore pass |
| DEV-07 | Bootlogo 480×640 apply and preview orientation | ✅ | RG28XX | Applied logo and rotated previews are upright |

## Recovery and robustness

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| ROBUST-01 | Stock `launcher.sh stop` SIGUSR1 does not terminate the NextUI boot shim | ⬜ | Code present | The shim deliberately ignores USR1 with `trap '' USR1`; no end-to-end result is recorded |
| ROBUST-02 | Dirty FAT card recovery via `fsck.fat -a` | ⬜ | Code present | Boot hook exists; no induced dirty-card recovery result is recorded |

## Capability boundaries

| ID | Test | Status | Evidence/device | Result and notes |
|---|---|---|---|---|
| CAP-01 | FN1/FN2/HOME assignable pak actions | ➖ | RG XX hardware/platform API | RG XX devices have no dedicated buttons; Settings hides the assignment UI |
| CAP-02 | Keep awake over USB data connection | ➖ | H700 platform API | `PLAT_isUSBConnected()` is a stub; charging-state power behavior is covered by POWER-04 |

## Test execution protocol

- Candidate builds must record BUILD-01, BUILD-02, and BUILD-03 against the actual
  artifact being promoted.
- Changes under `workspace/all/` require both H700 and tg5040 builds.
- A new device needs a short shared-runtime smoke plus tests for its hardware/model
  delta; it does not need the full matrix repeated without a specific risk.
- Re-run stock integration probes after an Anbernic firmware update because boot,
  model detection, WiFi, Bluetooth, and sysfs paths are firmware-coupled.
- When shared code adds a button or `PLAT_*` API, verify all platform definitions and
  run a full build; single-pak builds can miss translation units.
