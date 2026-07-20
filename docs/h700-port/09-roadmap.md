# 09 — Roadmap: beta1 → RC → 9.5/10 port

Hardware results recorded through 2026-07-20 (beta-prep test walkthrough).
Branch tip note may lag; treat the matrix in 08/10 as source of truth for status.
Ordered by impact within each section.

## beta1 vs RC gates

beta1 is allowed to ship with documented known issues and incomplete RC polish.
RC must clear the major defects below (or explicitly re-descope in writing).

### beta1 (ship now)
| Item | Status for beta1 |
|---|---|
| FBNeo missing-BIOS hard-lock | **Known issue** — happy path with BIOS OK; most users fine. Call out in release notes. Fix is **RC**, not beta1-blocking |
| MD Auto CPU | ✅ fixed |
| Screenshots, multi-panel UI scaling, headphone hardware route | ✅ |
| Closed-lid POWER, charging→light-sleep | ✅ accepted policy |
| Clean uninstall | ➖ not a test row — delete `dmenu.bin` is design-guaranteed reverse install |
| SIGUSR1 shutdown, dirty-card fsck | ⬜ untested OK for beta1 → **RC polish** |
| BT audio lifecycle remainder | ⚠ blocked by device BT scan; earlier AirPods playback pass stands |
| Overnight drain | ⚠ partial first data point; voltage/stock re-test is RC polish |
| Long-tail EXTRAS cores | ⬜ partial smoke only (see 10) |
| MU-style theme | ➖ install prerequisite only |

### RC (must close before release candidate)
| Gate | Exit condition |
|---|---|
| FBNeo error recovery | Missing-BIOS (and similar core hard-fail) returns control to MinArch menu/exit without power cycle; regression test preserved |
| Robustness polish | SIGUSR1 from `launcher.sh stop` end-to-end; dirty-card `fsck.fat` recovery smoke |
| Prefer | BT lifecycle revalidation when device scan works; overnight drain voltage vs stock; more EXTRAS core smoke; PS1 stays in RC smoke (stale-core regression) |
| Build | clean H700 + tg5040 builds + Jammy `settings.elf` ldd check on the candidate |

Not beta1 or RC gates unless scope changes: full RGcubexx matrix walk, H700-sized
overlay content packs, HDMI boot/EDID/model expansion, cross-platform controller
normalization, the inherited controller-only sample-rate detector, Pak Store/OTA,
per-panel display calibration, software HP jack icon, and a dedicated H700 toolchain
image.

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
   cases: POWER-while-closed is accepted policy (wakes light sleep only; deep sleep
   re-sleeps if lid still closed). Charging→light-sleep-only is accepted shared product
   policy for beta (2026-07-20).
2. **~~Decide muOS/stockmod coexistence policy~~ Closed (2026-07-20)** — reframed.
   The practical break is the stock **MU style** theme (not a separate product
   install): it makes `dmenu_ln` skip `/mnt/mmc/dmenu.bin`, so NextUI never starts
   and no in-shim warning can appear. Documented install prerequisite: use **old
   style** theme (`skeleton/BASE/README.txt`). The boot-shim “STOCK TARGET
   REQUIRED” path is effectively unreachable for the same override; leave it as
   defensive code, no further policy work for beta.
3. **Robustness gauntlet — scoped 2026-07-20.** Clean uninstall **removed** from the
   test matrix (delete `dmenu.bin` is sufficient by design). SIGUSR1 quit and
   dirty-SD `fsck` recovery are **RC polish** (untested OK for beta1). Overnight
   drain is partial (RG34XXSP ~7% over 8.5 h; voltage/stock re-test = RC polish).
   Battery % vs stock passes on RG34XXSP; icon-vs-Battery-pak desync after long sleep
   is post-RC hardening. Screenshots, box art, RetroAchievements, Recently Played,
   and the game switcher are tested-good on RG34XXSP. Pak Store and OTA remain out
   of scope.
4. **~~Document the `digital volume` inversion~~ Done (2026-07-09)** — `100 - val`
   is intentional because the control is an attenuator. Volume UI and levels are
   tested-good on RG40XXV and RG34XXSP from mute through 100% (05).

## P1 — Feature completeness

5. **RG34XXSP polish and runtime fixes** — the 720×480 UI, lid sleep/wake, manual
   governor changes, displaycal persistence, RetroAchievements, Files, Recently
   Played, game switcher, and the primary 8/16-bit systems are hardware-tested.
   Remaining for **RC**: fix FBNeo missing-BIOS hard-lock (happy path OK — known
   issue for beta1). Decide whether per-Emu `default-rg34xx.cfg` files are needed.
   Track core coverage in [10](10-core-game-matrix.md). (~~MD Auto CPU~~ fixed
   2026-07-20. ~~POWER through closed lid~~ and ~~charging suppresses deep sleep~~
   accepted policy 2026-07-20. ~~720×480 Bootlogo previews~~ fixed 2026-07-13 —
   see #13a.)
6. **~~RG28XX rotation validation~~ Done (2026-07-12)** — hardware answered both
   questions: the malifbdev-rot driver *does* rotate the GL path (SDL_ROTATION=1
   suffices end-to-end), and the two layers *did* double-rotate — the app-side
   `should_rotate` swap broke minarch Aspect/Fullscreen scaling and was removed
   (rotation is driver-level only now; postmortem in 04). UI, game scaling, and
   bootlogo apply are user-verified; bootlogo previews are rotated to boot
   orientation. Still open: decide whether `default-rg28xx.cfg` cfgs are needed.
7. **~~Rebase the h700 branch onto main~~ Done (2026-07-09)** — the branch includes
   main through the current shared-code baseline; rebase again before the release
   candidate only if main advances. (The alpha/tinted-
   bitmap question was solved: the glitches were the missing 64-bit libpng bundle,
   not main's alpha changes — 04.) Remaining: **confirm rendering stays clean on
   device with the rebased build**: the game switcher and box art are clean at
   720×480. Screenshots pass on RG34XXSP; multi-panel UI scaling passes (incl.
   community 720×720). Panel-matched overlay assets remain unshipped content.
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
9. **BT audio: finish stock-first validation (partial / blocked).** Clean RG40XXV
   stock provides BlueALSA 4.2.0, ALSA plugins/configuration, and D-Bus policy.
   Daemon startup, `org.bluealsa`, and SBC source endpoint registration pass; H700
   starts the stock daemon and exposes the sampling-rate setting. No BlueALSA, ALSA
   plugin, SBC, or BlueZ component is bundled. AirPods 4 ANC SBC playback passed on
   RG40XXV earlier (silence blockers: unsupported `delay 0` + zero BlueZ transport
   volume). **Auto reconnect was not tested.** As of 2026-07-20 BT powers on but
   scans nothing / cannot pair on stock and BaseOS — treat as device/firmware until
   proven otherwise; do not file as NextUI-only. When discovery works again: complete
   reconnect, game-switch, and suspend/resume. Settings 44100/48000 is a compatibility
   escape hatch (AirPods negotiated both). Keep controller-only sample-rate detection
   and raw controller button normalization as later cross-platform cleanups.
10. **480p UI polish pass** (04) — verdict from real use on RG40XXV and RG34XXSP:
    **good for alpha**. The 720×480 Home UI, Battery, Game Tracker, Input, Clock,
    Settings, on-screen keyboard, Files, and in-game menus all work well. Box art is
    clean. Screenshots pass on RG34XXSP; multi-panel UI scaling passes
    (640×480 / 720×480 / 480×640 local; 720×720 community). Panel-matched
    overlay assets are not shipped for H700 (only Brick 1024×768 GBA PNGs). Keep Files
    at PPU 2 for now; PPU 3 may be evaluated later for the RG34XX/SP panel density.
11. **~~Headphone jack detection~~ Closed for beta (2026-07-20)** (05) — hardware
    auto-mutes speaker and routes to headphones without NextUI. Software
    `SetJack` / HP icon / dual volume remains unwired on h700 (Brick has keymon
    jack monitoring; h700 does not). Treat HP-icon polish as optional post-beta.
12. **~~Investigate RG34XXSP auto CPU scaling~~ Fixed (2026-07-20)** — MD no longer
    slows down under Auto; former 480 MHz stall with some render settings is resolved
    (user-verified). Manual powersave/performance were already fine. No general
    frame-pacing, tearing, input-lag, or audio concerns from tested gameplay.

12a. **Build out the core/game matrix** — the initial RG34XXSP results are now in
    [10](10-core-game-matrix.md). GB/GBC/GBA/FC/SFC, MD, and PS1 pass; the former PS1
    launch failures were a stale/corrupted core artifact fixed by a clean rebuild.
    FBNeo with BIOS is OK; missing-BIOS hard-lock is a **beta1 known issue** and an
    **RC must-fix**. A2600, MGBA, and SMS also pass (2026-07-20). Expand coverage
    across remaining EXTRAS systems when practical (not beta1-blocking).
13. Later: RGcubexx bring-up (720×720 wired; general UI scaling already
    community-validated — expand to a fuller functional/core walk when a device is
    available), optional H700-sized overlay content packs, HDMI out (done,
    user-validated on RG40XXV incl. in-game hotplug both directions; remaining:
    other models, boot-with-cable, odd EDIDs — 04), RG35XX-family variants
    (displaycal presets
    + `DEVICE=rg35xx` mapping already plumbed — 04), Panel-Fix tool, **per-panel
    displaycal calibration** (measure each device, fill in the neutral presets in
    displaycal.h; confirm exact `RGXX_MODEL` strings for RG35xx family / RG40xxH
    while at it).

13a. **Bootlogo pak: working on 3 of 4 resolutions** — `Bootlogo.pak` builds and
    ships for h700: full tg5040 preset catalog regenerated as 24-bit BMPs per panel
    resolution (`640x480`, `720x480`, `480x640` rotated for RG28XX, `720x720`),
    folder picked via `$DEVICE`; writes `bootlogo.bmp` to `mmcblk0p2` and backs up
    the stock logo as `original.bmp` on first apply (02, 03). User-tested on
    RG40XXV: 640×480 apply + `original.bmp` backup verified, and the original appears
    in the carousel. Restore was not explicitly selected but uses the same apply path;
    the post-apply reboot is a bit slow but acceptable. RG34XXSP 720×480 previews
    were absent with the alpha1.1 binary; fixed with the current build
    (2026-07-13, verified on-device — 04) and the pak now has load/path diagnostics,
    an on-screen empty state, and an empty-list apply guard. RG28XX 480×640 apply is
    user-verified (logo upright at boot) and previews are now rotated to match boot
    orientation (`BOOTLOGO_PREVIEW_ROTATE_CW`, 04). SP backup/restore pass
    (user-verified 2026-07-20). Remaining: the 720×720 path (no local cube device;
    community UI scaling only so far).

## P2 — Cleanup / refactors / simplifications

14. **~~Remove the crash-loop network behavior~~ Done (2026-07-09)** — after 5
    `nextui.elf` crashes, launch.sh now logs the limit, removes `/tmp/nextui_exec`,
    and powers off instead of bringing up WiFi/SSH.
15. **~~Dedupe boot/installer shell code~~ Done (2026-07-09)** — `boot/boot.sh`
    and `install/boot.sh` now share mount/compat/log helpers via
    `workspace/h700/shim-common.sh`, packaged in both the self-extracting boot shim
    payload and `.tmp_update/h700/`.
16. **Dedicated h700 toolchain image, revisited — deferred for alpha** (01) — a thin
    `FROM tg5040-toolchain` layer pre-baking the pinned SDL2 removes the pitfall
    classes 1–3 in 01 structurally and cuts CI time. Do
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
- BT audio headset playback/reconnect/suspend matrix passes on supported stock firmware
- No debug behaviors in the release path; docs (README.txt) match actual behavior
- CI: h700 + tg5040 both build on every PR touching shared code, with the ldd check
