# 09 — Roadmap: from alpha candidate to a 9.5/10 port

Everything below is verified against branch tip `4c754b7`, with hardware results
recorded through 2026-07-10.
Ordered by impact within each section.

## P0 — Correctness / robustness

1. **~~Fix unreliable sleep/wake~~ Done (2026-07-09)** — was four stacked bugs, all
   fixed and user-verified on RG34XXSP (see 06 for the full postmortem): the suspend
   script died on `set -o pipefail` (unsupported shell → non-zero exit → poweroff
   instead of suspend), the persistent `wake_fd` buffered the sleep-triggering
   power-key release (instant bounce-back on every 2nd+ sleep), lid close could never
   satisfy the sleep condition (now `PWR_requestSleep()`), and wake froze the UI
   ~5–10 s (ALSA PCM held open across suspend takes SDL ~9 s to close — now closed in
   `PWR_enterSleep`; plus wifi/bt restart moved to `after_async &`). Unblocks the
   overnight-drain test. RG40XXV re-test, in-game sleep/resume, and power-off →
   power-on → running-game resume have since been verified too. Two RG34XXSP edge
   cases remain: POWER currently wakes the unit while its lid is closed, and charging
   intentionally keeps the shared power path in light sleep instead of deep suspend.
2. **Decide muOS/stockmod coexistence policy** (`boot/boot.sh` — currently shows
   "STOCK TARGET REQUIRED" splash but *continues booting*; launch.sh drops
   stockmod-warning.txt). Either hard-fail with the splash held on screen, or
   document why continuing is safe. A user with stockmod installed currently gets a
   confusing half-boot.
3. **Run the remaining robustness gauntlet** (08 matrix ⬜ rows): clean uninstall,
   SIGUSR1 quit, dirty-SD fsck recovery, battery accuracy, screenshots, and overnight
   drain. Box art, RetroAchievements, Recently Played, and the game switcher are
   tested-good on RG34XXSP.
   Pak Store and OTA update are explicitly excluded from the alpha scope; testing is
   not applicable and they are not alpha release gates.
4. **~~Document the `digital volume` inversion~~ Done (2026-07-09)** — `100 - val`
   is intentional because the control is an attenuator. Volume UI and levels are
   tested-good on RG40XXV and RG34XXSP from mute through 100% (05).

## P1 — Feature completeness

5. **RG34XXSP polish and runtime fixes** — the 720×480 UI, lid sleep/wake, manual
   governor changes, displaycal persistence, RetroAchievements, Files, Recently
   Played, game switcher, and the primary 8/16-bit systems are hardware-tested.
   Remaining: fix POWER waking through a closed lid; decide whether charging should
   continue to suppress deep sleep; diagnose missing 720×480 Bootlogo previews, PS1
   launch crashes, the FBNeo missing-BIOS lockup, and MD auto-governor/render-setting
   sensitivity; decide whether per-Emu `default-rg34xx.cfg` files are needed. Track
   core coverage in [10](10-core-game-matrix.md).
6. **RG28XX rotation validation** — plumbing exists on both layers (SDL_ROTATION=1
   env + `should_rotate` GL path, 04). Verify on hardware that the malifbdev-rot
   patch covers GL contexts and that the two layers don't double-rotate; add
   `default-rg28xx.cfg` cfgs.
7. **~~Rebase the h700 branch onto main~~ Done (2026-07-09)** — the branch was rebased
   onto main including the alpha-blending work. Upstream main has advanced since, so
   rebase once more before the final merge/release candidate. (The alpha/tinted-
   bitmap question was solved: the glitches were the missing 64-bit libpng bundle,
   not main's alpha changes — 04.) Remaining: **confirm rendering stays clean on
   device with the rebased build**: the game switcher and box art are clean at
   720×480. Screenshots and resolution-specific overlays remain to be checked.
8. **Audit Brick-era feature assumptions in shared UI** — NextUI only ever targeted
   the Brick / Smart Pro, and several UI pieces hardcode that hardware. Known cases
   on RG XX:
   - **~~Input tester pak~~ Done (2026-07-09)** — L3/R3 pills were drawn on every
     device because `CODE_L3/R3` were unconditional in platform.h, and `is_rg34xx`
     wrongly stripped L3/R3 from the RG34XXSP. Now `detect_device()` sets
     `dev_has_lstick`/`dev_has_rstick` per model (verified matrix in 03: 35XXH/
     35XXPro/34XXSP/40XXH/cube dual, 40XXV left-only, 28XX/34XX/35XX+/2024/SP
     none — note the RG40XXV has *one* stick, clickable as L3) and those flags
     gate CODE_L3/R3, JOY_L3/R3, and the AXIS_* macros. Zero shared-code changes;
     the Input pak adapts via its existing `has_*` derivation. Tested-good on
     RG40XXV and RG34XXSP. The pak shows L3/R3 click state, not analog movement;
     that is its existing cross-platform behavior and is not an H700 port task.
     Other variants remain alpha hardware-validation targets. Remaining:
     confirm exact `RGXX_MODEL` strings for the RG35xx family / RG40xxH on hardware.
   - **~~Fn switch option~~ Done (2026-07-09)** — the sliding Fn button doesn't
     exist on any RG XX device, so the settings capability predicate now excludes
     h700 while retaining the option on tg5040/tg5050.
   - **~~Display settings~~ Done (2026-07-09, `0e60efd`)** — visually verified on
     RG40XXV. The dead enhance
     controls (contrast/saturation/exposure) are now hidden on h700; displaycal
     reboot persistence fixed (was clobbered to defaults every boot); per-model
     displaycal preset plumbing added for the full 11-device H700 family (all
     neutral/disabled until panels are measured); default brightness raised to 4;
     `RG35xx*` now maps to `DEVICE=rg35xx` in launch.sh (see 04). Remaining
     nit: syncsettings still "restores" the no-op enhance values on resume.
   Worth a systematic sweep: grep settings.cpp / paks for capability flags that
   default to "present" and decide each for h700.
9. **BT audio: ship bluealsa or formally drop it.** The gate-off is clean and
   reversible (drop a `bluealsa` binary in `.system/h700/bin` and the path lights
   up). Building bluez-alsa in the toolchain is the last piece of tg5040 feature
   parity. If dropped instead, remove the dormant bt_init/audiomon plumbing.
10. **480p UI polish pass** (04) — verdict from real use on RG40XXV and RG34XXSP:
    **good for alpha**. The 720×480 Home UI, Battery, Game Tracker, Input, Clock,
    Settings, on-screen keyboard, Files, and in-game menus all work well. Box art is
    clean. Screenshots and resolution-specific overlays remain untested. Keep Files
    at PPU 2 for now; PPU 3 may be evaluated later for the RG34XX/SP panel density.
11. **Headphone jack detection** (05) — investigate how stock switches speaker/HP on
    a live device; may be hardware auto-mute (= nothing to do). Cheap to answer,
    closes a matrix row either way.
12. **Investigate RG34XXSP auto CPU scaling** — manual in-game governor changes work,
    but MD can slow down when Auto settles near 480 MHz with some shader/render
    combinations. Stock shader + 3× scale + linear interpolation raised Auto to about
    720 MHz and ran smoothly; Powersave and Performance selected about 1100/1500 MHz
    and were also smooth. Determine whether this is workload detection, a core/render
    interaction, or an Auto policy bug. There are no general frame-pacing, tearing,
    input-lag, or audio concerns from tested gameplay.

12a. **Build out the core/game matrix** — the initial RG34XXSP results are now in
    [10](10-core-game-matrix.md). GB/GBC/GBA/FC/SFC pass; PS1 crashes back to Home for
    every tested title; FBNeo needs a valid BIOS retest and must not trap MinArch when
    BIOS is missing; MD needs the Auto-scaling investigation above. Expand coverage
    across the remaining shipped systems and at least one additional H700 model.
13. Later: RGcubexx bring-up (720×720, wired but no device is available locally;
    external validation is an explicit purpose of the alpha), HDMI out (`SetHDMI()` is
    a no-op; mechanism documented in 04), RG35XX-family variants (displaycal presets
    + `DEVICE=rg35xx` mapping already plumbed — 04), Panel-Fix tool, **per-panel
    displaycal calibration** (measure each device, fill in the neutral presets in
    displaycal.h; confirm exact `RGXX_MODEL` strings for RG35xx family / RG40xxH
    while at it).

13a. **Bootlogo pak: partial** — `Bootlogo.pak` builds and ships for
    h700: full tg5040 preset catalog regenerated as 24-bit BMPs per panel
    resolution (`640x480`, `720x480`, `480x640` rotated for RG28XX, `720x720`),
    folder picked via `$DEVICE`; writes `bootlogo.bmp` to `mmcblk0p2` and backs up
    the stock logo as `original.bmp` on first apply (02, 03). User-tested on
    RG40XXV: 640×480 apply + `original.bmp` backup verified, and the original appears
    in the carousel. Restore was not explicitly selected but uses the same apply path;
    the post-apply reboot is a bit slow but acceptable. On RG34XXSP, the 720×480 pak
    shows no preview images. The packaged BMPs are valid, correctly sized, and
    non-black, so this is a runtime loading/rendering issue rather than blank assets;
    applying or restoring was not attempted there. The 480×640 and 720×720 paths
    still need hardware testing.

## P2 — Cleanup / refactors / simplifications

14. **~~Remove the crash-loop network behavior~~ Done (2026-07-09)** — after 5
    `nextui.elf` crashes, launch.sh now logs the limit, removes `/tmp/nextui_exec`,
    and powers off instead of bringing up WiFi/SSH.
15. **~~Dedupe boot/installer shell code~~ Done (2026-07-09)** — `boot/boot.sh`
    and `install/boot.sh` now share mount/compat/log helpers via
    `workspace/h700/shim-common.sh`, packaged in both the self-extracting boot shim
    payload and `.tmp_update/h700/`.
16. **Dedicated h700 toolchain image, revisited — deferred for alpha** (01) — a thin
    `FROM tg5040-toolchain` layer pre-baking the pinned SDL2 (and bluealsa if #9
    ships) removes the pitfall classes 1–3 in 01 structurally and cuts CI time. Do
    it when the next external dep lands.
17. **~~Trim remaining tg5040 residue in the skeleton~~ Done (2026-07-09)** —
    h700 paks/cfgs are clear of `default-brick`, `reboot_next`, `libUMP`, bundled
    `libasound`, and TrimUI-only skeleton comments; stale commented `&>` redirects
    in the h700 tool launchers were replaced with POSIX `> ./log.txt 2>&1` logging.
18. **~~settings glib check~~ Done (2026-07-09)** — CI/release now build h700 and
    run `workspace/h700/check-settings-ldd.sh` in a Jammy container against
    `settings.elf` with the h700 bundled lib path, failing on unresolved libs or
    missing gio/glib linkage.
19. **Wire richer battery metrics — deferred for alpha** (optional) —
    `time_to_empty_now`/`voltage_now`/`charge_counter` exist on the PMIC; batmon's
    SQLite would get better data for a trivial PLAT extension, but the alpha cleanup
    keeps battery schemas unchanged.

## What "9.5/10" looks like

- Sleep/wake solid across a 20-cycle soak + overnight drain ≤ stock + 1% (P0 #1)
- Every 08-matrix row ✅ on RG40XXV, lid fixed on RG34XXSP, and the functional set ✅
  on RG34XXSP + RG28XX
- BT audio shipped or explicitly descoped in README
- No debug behaviors in the release path; docs (README.txt) match actual behavior
- CI: h700 + tg5040 both build on every PR touching shared code, with the ldd check
