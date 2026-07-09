# 09 — Roadmap: from working beta to a 9.5/10 port

Everything below is verified against branch tip `752cefe8` (2026-07-09). Ordered by
impact within each section.

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

5. **RG34XXSP bring-up** — mostly config already wired (`is_rg34xx` 720×480, lid).
   Needs hardware in hand: hallkey semantics, lid sleep/wake, 720×480 UI pass, and
   per-Emu `default-rg34xx.cfg` device cfgs (none exist yet — only `default.cfg`).
6. **RG28XX rotation validation** — plumbing exists on both layers (SDL_ROTATION=1
   env + `should_rotate` GL path, 04). Verify on hardware that the malifbdev-rot
   patch covers GL contexts and that the two layers don't double-rotate; add
   `default-rg28xx.cfg` cfgs.
7. **Resolve the alpha/tinted-bitmap question** (04) — the reverted experiment means
   H700 rides a rendering path that *looks* right but differs from what was attempted.
   Reproduce the original artifact, root-cause it (likely SDL2/SDL2_image
   pixel-format or blend defaults in our custom build), and either fix in the SDL
   build (preferred — keeps `workspace/all/` clean) or land a properly-gated shared
   fix.
8. **BT audio: ship bluealsa or formally drop it.** The gate-off is clean and
   reversible (drop a `bluealsa` binary in `.system/h700/bin` and the path lights
   up). Building bluez-alsa in the toolchain is the last piece of tg5040 feature
   parity. If dropped instead, remove the dormant bt_init/audiomon plumbing.
9. **480p UI audit** (04) — systematic pass over fonts, pills, quick switcher, long
   titles at 640×480 and 720×480. NextUI hadn't rendered at 480p in ~2 years;
   browsing looks fine but nobody has checked the corners.
10. **Headphone jack detection** (05) — investigate how stock switches speaker/HP on
    a live device; may be hardware auto-mute (= nothing to do). Cheap to answer,
    closes a matrix row either way.
11. **Measure real panel refresh** — `SCREEN_FPS 60.0` is assumed. A vsync-timing
    test per device takes minutes and protects frame pacing math.
12. Later: RGcubexx bring-up (720×720, wired but untested), HDMI out (`SetHDMI()` is
    a no-op; mechanism documented in 04), RG35XX-family variants, Panel-Fix tool.

## P2 — Cleanup / refactors / simplifications

13. **Gate the crash-loop network behavior**: after 5 nextui crashes, launch.sh
    brings up WiFi+SSH for 300 s unconditionally — great for beta, a mild security
    surprise for release. Tie it to the `debug-keep-network` flag (or a
    `.userdata` setting) before calling the port done.
14. **Dedupe boot/installer shell code** — `boot/boot.sh` and `install/boot.sh`
    share mount/splash/log idioms with subtle divergence risk. Extract the common
    helpers into one sourced file inside the shim payload.
15. **Dedicated h700 toolchain image, revisited** (01) — a thin
    `FROM tg5040-toolchain` layer pre-baking the pinned SDL2 (and bluealsa if #8
    ships) removes the pitfall classes 1–3 in 01 structurally and cuts CI time. Do
    it when the next external dep lands.
16. **Trim remaining tg5040 residue in the skeleton** — sweep h700 paks/cfgs for
    trimui-isms and stale comments (the big items — reboot_next, -brick cfgs,
    libUMP/libasound bundling — are already gone).
17. **settings glib check** — settings.elf links the sysroot's glib and empirically
    loads against the device's 2.72; add a one-line `ldd`-against-stock-rootfs CI
    check (01's bundling rule) so an SDK bump can't silently break it.
18. **Wire richer battery metrics** (optional) — `time_to_empty_now`/`voltage_now`/
    `charge_counter` exist on the PMIC; batmon's SQLite would get better data for a
    trivial PLAT extension.

## What "9.5/10" looks like

- Sleep/wake solid across a 20-cycle soak + overnight drain ≤ stock + 1% (P0 #1)
- Every 08-matrix row ✅ on RG40XXV, and the functional set ✅ on RG34XXSP + RG28XX
- BT audio shipped or explicitly descoped in README
- No debug behaviors in the release path; docs (README.txt) match actual behavior
- CI: h700 + tg5040 both build on every PR touching shared code, with the ldd check
