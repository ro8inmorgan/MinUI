# 09 — Roadmap: from working beta to a 9.5/10 port

Everything below is verified against branch tip `752cefe8` (updated 2026-07-09).
Ordered by impact within each section.

## P0 — Correctness / robustness

1. **Fix unreliable sleep/wake** (the #1 defect — see 06 for the state of play and
   suspects). All previously-identified software suspects are already fixed, so this
   needs real debugging: instrument `skeleton/SYSTEM/h700/bin/suspend` with
   breadcrumbs to distinguish hang-on-entry from hang-on-exit, compare our
   pre-suspend service/driver state against stock's sleep path, and reconsider the
   5-try retry loop (a retry against a half-suspended SoC may itself wedge). Blocks
   the overnight-drain test and any credible "sleep is a headline feature" claim.
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
4. **Document-or-fix the `digital volume` inversion** (`msettings.c` `100 - val`):
   it's confirmed correct by ear, but the only in-tree explanation is a terse
   comment. One paragraph in the code explaining the attenuator semantics + garbage
   TLV prevents a future "cleanup" from re-breaking audio.

## P1 — Feature completeness

5. **RG34XXSP lid fix + polish** — general bring-up is now user-tested and works like
   RG40XXV, including the 720×480 UI path. Remaining SP-specific work: fix lid
   sleep/wake (currently the screen stays on), probe hallkey semantics/polarity, verify
   power-key-while-closed behavior, and decide whether per-Emu `default-rg34xx.cfg`
   device cfgs are needed (none exist yet — only `default.cfg`).
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
   - **Display settings** expose brightness/saturation/contrast/exposure enhance
     controls that **do nothing on RG XX**. What actually works: LCD backlight
     brightness, color temperature, white-point correction (displaycal), RGB
     tuning. Gate the dead controls per-platform (and stop syncsettings from
     "restoring" values that have no effect).
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
    a no-op; mechanism documented in 04), RG35XX-family variants, Panel-Fix tool.

## P2 — Cleanup / refactors / simplifications

14. **Gate the crash-loop network behavior**: after 5 nextui crashes, launch.sh
    brings up WiFi+SSH for 300 s unconditionally — great for beta, a mild security
    surprise for release. Tie it to the `debug-keep-network` flag (or a
    `.userdata` setting) before calling the port done.
15. **Dedupe boot/installer shell code** — `boot/boot.sh` and `install/boot.sh`
    share mount/splash/log idioms with subtle divergence risk. Extract the common
    helpers into one sourced file inside the shim payload.
16. **Dedicated h700 toolchain image, revisited** (01) — a thin
    `FROM tg5040-toolchain` layer pre-baking the pinned SDL2 (and bluealsa if #9
    ships) removes the pitfall classes 1–3 in 01 structurally and cuts CI time. Do
    it when the next external dep lands.
17. **Trim remaining tg5040 residue in the skeleton** — sweep h700 paks/cfgs for
    trimui-isms and stale comments (the big items — reboot_next, -brick cfgs,
    libUMP/libasound bundling — are already gone).
18. **settings glib check** — settings.elf links the sysroot's glib and empirically
    loads against the device's 2.72; add a one-line `ldd`-against-stock-rootfs CI
    check (01's bundling rule) so an SDK bump can't silently break it.
19. **Wire richer battery metrics** (optional) — `time_to_empty_now`/`voltage_now`/
    `charge_counter` exist on the PMIC; batmon's SQLite would get better data for a
    trivial PLAT extension.

## What "9.5/10" looks like

- Sleep/wake solid across a 20-cycle soak + overnight drain ≤ stock + 1% (P0 #1)
- Every 08-matrix row ✅ on RG40XXV, lid fixed on RG34XXSP, and the functional set ✅
  on RG34XXSP + RG28XX
- BT audio shipped or explicitly descoped in README
- No debug behaviors in the release path; docs (README.txt) match actual behavior
- CI: h700 + tg5040 both build on every PR touching shared code, with the ldd check
