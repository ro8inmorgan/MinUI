# 09 — Roadmap: from working beta to a 9.5/10 port

Everything below is verified against branch tip `0e60efd` (updated 2026-07-09).
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
   overnight-drain test.
2. **Decide muOS/stockmod coexistence policy** (`boot/boot.sh` — currently shows
   "STOCK TARGET REQUIRED" splash but *continues booting*; launch.sh drops
   stockmod-warning.txt). Either hard-fail with the splash held on screen, or
   document why continuing is safe. A user with stockmod installed currently gets a
   confusing half-boot.
3. **Run the untested-robustness gauntlet** (08 matrix ⬜ rows): clean uninstall,
   SIGUSR1 quit, dirty-SD fsck recovery, battery accuracy/charging, screenshots/
   recents/box art, RetroAchievements, Pak Store, OTA update. Each is implemented;
   none has been exercised. OTA especially — a broken update path is the worst
   post-release bug class.
4. **~~Document the `digital volume` inversion~~ Done (2026-07-09)** — `100 - val`
   is intentional because the control is an attenuator. Volume UI and levels are
   tested-good on RG40XXV and RG34XXSP from mute through 100% (05).

## P1 — Feature completeness

5. **RG34XXSP lid fix + polish** — general bring-up is now user-tested and works like
   RG40XXV, including the 720×480 UI path. ~~Fix lid sleep/wake~~ Done (2026-07-09):
   lid close sleeps, lid open wakes from screen-off (power key needed after deep
   suspend, matching stock — hallkey polarity verified 1=open; see 06). Remaining:
   decide whether per-Emu `default-rg34xx.cfg` device cfgs are needed (none exist
   yet — only `default.cfg`).
6. **RG28XX rotation validation** — plumbing exists on both layers (SDL_ROTATION=1
   env + `should_rotate` GL path, 04). Verify on hardware that the malifbdev-rot
   patch covers GL contexts and that the two layers don't double-rotate; add
   `default-rg28xx.cfg` cfgs.
7. **~~Rebase the h700 branch onto main~~ Done (2026-07-09)** — the branch now sits
   on top of current main including the alpha-blending work. (The alpha/tinted-
   bitmap question was solved: the glitches were the missing 64-bit libpng bundle,
   not main's alpha changes — 04.) Remaining: **confirm rendering stays clean on
   device with the rebased build** (blit-heavy screens: browse list with box art,
   game switcher, overlays).
8. **Audit Brick-era feature assumptions in shared UI** — NextUI only ever targeted
   the Brick / Smart Pro, and several UI pieces hardcode that hardware. Known cases
   on RG XX:
   - **Input tester pak** shows the Brick's button layout: no analog sticks
     rendered, R3 assumed always present. Needs device-aware layout (RG40XXV has
     dual sticks + L3/R3; 34XX/28XX have none).
   - **Fn switch option** in the settings app — the sliding Fn button doesn't exist
     on any RG XX device; hide/gate it for h700.
   - **~~Display settings~~ Done (2026-07-09, `0e60efd`)** — the dead enhance
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
10. **480p UI polish pass** (04) — verdict from real use on RG40XXV/RG34XXSP:
    **mostly fine, OK for alpha**. No systemic layout breakage; known concrete issue: in some paks
    (e.g. the Battery pak) the button-hint pills are large enough to overlap each
    other. Before release, audit every shipped-by-default UI surface at 640×480
    and 720×480 and fix pill/hint sizing where cramped.
11. **Headphone jack detection** (05) — investigate how stock switches speaker/HP on
    a live device; may be hardware auto-mute (= nothing to do). Cheap to answer,
    closes a matrix row either way.
12. **Measure real panel refresh** — `SCREEN_FPS 60.0` is assumed. A vsync-timing
    test per device takes minutes and protects frame pacing math.
13. Later: RGcubexx bring-up (720×720, wired but untested), HDMI out (`SetHDMI()` is
    a no-op; mechanism documented in 04), RG35XX-family variants (displaycal presets
    + `DEVICE=rg35xx` mapping already plumbed — 04), Panel-Fix tool, **per-panel
    displaycal calibration** (measure each device, fill in the neutral presets in
    displaycal.h; confirm exact `RGXX_MODEL` strings for RG35xx family / RG40xxH
    while at it).

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
