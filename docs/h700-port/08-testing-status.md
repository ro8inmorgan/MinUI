# 08 — Testing Status & Regression Guardrails

## Test fleet

| Device | Evidence | Role |
|---|---|---|
| RG40XXV | Probed live | Primary bring-up device |
| RG34XXSP | User-tested | General parity with RG40XXV; sleep/wake + lid validated here |
| RG28XX | User-tested | Rotation validated: UI, game scaling, bootlogo apply working |
| RGcubexx | Community-tested (Discord) | External validation target; general UI scaling confirmed 720×720 |
| TrimUI Brick TG5040 | Probed live | Regression reference (behavioral 1:1 comparisons) |

**Dev-loop tips:**
- The stock OS runs sshd with a full Ubuntu userland — iterate by `scp`ing freshly
  built `.elf`s into `.system/h700/bin` and restarting the launch loop. No SD-card
  shuffling after first install.
- Repeated `nextui.elf` crashes now log the crash limit and power off. Use TF2 logs
  for crash-loop debugging; the release runtime powers off instead of starting SSH.

## Validation matrix (status as of 2026-07-20, RG40XXV unless noted)

✅ tested & passed ⚠️ tested, has issues ⬜ untested ➖ explicitly out of alpha scope ✖ not implemented

### Functional
| Item | Status |
|---|---|
| Cold boot to NextUI, stable across 10+ reboots, reasonable boot time | ✅ |
| Browse/launch/quit games across system pak cores; save/load state; in-game menu | ✅ |
| Fast resume (BTN_RESUME) + auto-resume after power loss | ✅ |
| Brightness 0–10 + colortemp ramps; persist across reboot | ✅ |
| MENU short-tap → shortcuts overlay vs. hold → brightness (compound-tap fix, see 00/03) | ✅ tested on RG34XXSP; firmware quirk is common to the H700 line, expected fine on all RG XX |
| Displaycal RGB gains: act, persist, survive sleep + game launch | ✅ |
| Displaycal survives reboot (was clobbered to defaults every boot; fixed 2026-07-09) | ✅ verified on RG40XXV and RG34XXSP |
| Fresh-install defaults: brightness 4, displaycal off/neutral | ✅ verified on RG40XXV (deleted msettings.bin + reboot) |
| Dead enhance controls (contrast/saturation/exposure) hidden on h700 | ✅ code-gated and visually verified on RG40XXV 2026-07-09 |
| Fn-switch settings hidden on h700 | ✅ capability-gated; RG XX devices have no Fn switch |
| FN1/FN2/HOME assignable pak actions | ➖ not supported (no dedicated keys; `BTN_FN*` stubs) |
| Keep awake over USB | ➖ not supported (`PLAT_isUSBConnected` stub; tg5040-only UI) |
| Shaders (all shipped .glsl), overlays, effects on Mali-G31 | ✅ |
| Volume UI + levels, mute through 100% | ✅ tested on RG40XXV and RG34XXSP; no known issues |
| Rumble (moto on/off) | ✅ |
| WiFi scan/connect/forget | ✅ |
| WiFi survives sleep; NTP sync | ✅ |
| BT adapter init + controller pairing/attachment | ✅ RG40XXV: vendor UART attach creates `hci0`; stock BlueZ 5.66 discovers, pairs, trusts, and connects a DualSense; NextUI opens it as an SDL joystick plus `/dev/input/event3`; five restart cycles passed without duplicate attach/daemon processes. Raw button semantics remain a platform-agnostic tg5040/tg5050/H700 mapping issue and are out of scope for this branch |
| Deep sleep (auto/manual) + wake | ✅ core paths work on RG34XXSP and RG40XXV, including in-game resume and power-off auto-resume. RG34XXSP closed-lid POWER behavior accepted as policy (light sleep only — see lid row). Charging suppresses deep sleep by shared product policy — accepted for beta (06) |
| Files app (NextCommander): input, scaling, first-frame render | ✅ tested on RG40XXV and RG34XXSP; PPU 2 retained for alpha |
| Input pak: per-device stick layout | ✅ RG40XXV shows working L3 only; RG34XXSP shows working L3/R3. Analog movement is not visualized by the existing Input pak and is out of scope for this port |
| Recently Played + game switcher | ✅ tested on RG40XXV and RG34XXSP |
| Screenshots | ✅ tested on RG34XXSP |
| Box art | ✅ tested at 720×480 on RG34XXSP |
| HDMI output | ⚠️ RG40XXV + TV passes menu/game output and in-game hotplug in both directions, including audio routing. Boot-with-cable, other H700 models, and displays that reject the fixed 1080p60 mode remain untested; HDMI is a stretch feature rather than a beta gate (04) |
| Clean uninstall (delete dmenu.bin → pristine stock) | ➖ removed from test matrix — design-guaranteed: without `/mnt/mmc/dmenu.bin`, stock `dmenu_ln` never launches NextUI on RG XX (hijack is that one file). No separate end-to-end uninstall exercise required for beta/RC (02) |
| Battery % accuracy vs stock | ✅ percentage matches stock on fresh boot (RG34XXSP). After a ~9 h sleep/wake cycle the top-right battery icon and Battery pak were briefly out of sync with each other — level itself is fine; treat the dual-source desync as a future hardening item, not a beta gate |
| Charging detection/indicator; sleep while charging | ✅ indicator works on RG34XXSP; while charging, only light/screen-off sleep is allowed — shared product policy, **accepted for beta** (not a failed deep-sleep attempt) (06) |
| Overnight drain (8 h mem-sleep vs stock baseline) | ⚠️ partial — RG34XXSP deep sleep ~8.5 h: 40% → 33% (~7% capacity). Higher than hoped; re-test with `voltage_now` (not %) and a stock OS baseline before treating as pass/fail |
| RetroAchievements login + unlock | ✅ works on RG34XXSP; on-screen credential keyboard also verified |
| Pak Store install | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| OTA update flow | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| BT controller semantic mapping | ⚠ pairing and input attachment pass; raw button indices vary by controller and require a later cross-platform tg5040/tg5050/H700 normalization change |
| Bootlogo pak, RG40XXV 640×480 | ✅ carousel/apply/backup tested; restore inferred via same apply path |
| Bootlogo pak, RG34XXSP 720×480 | ✅ fixed 2026-07-13 — all 23 presets load and render with the current build. First-apply `original.bmp` backup and restore also pass (user-verified 2026-07-20). Pak logs path/count/load errors and shows the searched path on screen when empty; apply guarded against an empty list |
| Bootlogo pak, RG28XX 480×640 | ✅ apply user-tested: logo renders upright at boot. Previews were shown panel-native (90° off) and are now rotated to boot orientation (`BOOTLOGO_PREVIEW_ROTATE_CW`) |
| BT A2DP audio | ⚠ partial — earlier RG40XXV pass: AirPods 4 ANC pair/connect + user-audible SBC game audio; internal speaker mutes/restores correctly (blockers were stock `delay 0` rejection + zero BlueZ transport volume; fixed with stock BlueALSA + native volume). **Auto reconnect, menu/game and game/game switching, five suspend/resume cycles, and case/disconnect speaker-restore revalidation are blocked** as of 2026-07-20: BT powers on but scans no devices / cannot pair on **both stock OS and BaseOS** (device/firmware environment issue, not NextUI-specific). Re-run the full lifecycle matrix once scanning works again (05/07) |
| BT maximum sampling rate | ⚠ Matches tg5040 and defaults to 48000 Hz. AirPods accepted both 44100 and 48000 during probing (escape hatch, not the silence fix). **Settings 44100/48000 change+persist revalidation blocked** by the same device-wide scan/pair outage (stock + BaseOS). The inherited connection test also sees controller-only ACL links; at the 48000 default this is harmless; audio-specific detector is a cross-platform cleanup, not an H700 beta blocker |
| Headphone jack detection | ✅ hardware path pass — plug-in auto-mutes the speaker and routes audio to headphones with no software help (RG34XXSP / H700 line). NextUI software jack state (`SetJack` / separate HP volume / HP icon) is **not** wired on h700 (keymon never monitors jack; `GetJack` stays 0). That is Brick-style polish only; correct routing does not depend on it. Optional follow-up if HP-icon or dual-volume is desired (05) |
| RG34XXSP: general H700 port + 720×480 UI | ✅ Battery, Game Tracker, Input, Clock, Settings, Files, keyboard, box art, game switcher, and in-game menus work well; UI scaling good at 720×480 |
| RG34XXSP: lid sleep/wake | ✅ accepted documented policy — lid close/open work. POWER can wake **light** sleep while the lid is closed; in **deep** sleep, POWER that briefly wakes the unit returns it to sleep if the lid is still closed. Not treated as a beta defect (06) |
| RG28XX: rotated UI + games | ✅ user-tested — UI correct; minarch Aspect/Fullscreen were broken by app-side double-rotation, fixed by removing `should_rotate` (rotation is driver-level only — 04) |
| RGcubexx: 720×720 | ⚠️ partial external validation only — general UI scaling passes (Discord). No local device; no further testing as of 2026-07-20. Full functional matrix remains open for external testers |
| Resolution-specific overlays (panel-matched assets) | ➖ no H700 panel-sized overlay pack ships today. Tree has empty per-system folders plus two GBA PNGs at **1024×768** (Brick). Overlay *pipeline* already ✅ under shaders/effects; panel-matched content is a content follow-up, not a beta gate |

### Performance
| Item | Status |
|---|---|
| GBA/SNES full speed with vsync, no audio underruns | ✅ on RG40XXV and in tested RG34XXSP games |
| PS1 launch/performance | ✅ on RG40XXV and RG34XXSP. The apparent RG34XXSP launch regression was a stale/corrupted core artifact; a clean core rebuild fixed it |
| Frame pacing / tearing / input lag vs stock RA | ✅ gameplay-tested on RG40XXV; no visible pacing, tearing, latency, or audio issues (not instrumented) |
| Manual governor changes don't stutter audio | ✅ tested in-game on RG34XXSP |
| Auto governor load/frequency selection | ✅ MD Auto CPU slowdown fixed (user-verified 2026-07-20); manual powersave/performance remain smooth |

### RG34XXSP core/game bring-up

Detailed results and the remaining system backlog are tracked in
[10-core-game-matrix.md](10-core-game-matrix.md).

| System/core | Status |
|---|---|
| GB / GBC / GBA / FC / SFC | ✅ games launch and play perfectly |
| A2600 / MGBA / SMS | ✅ launch and play (2026-07-20 smoke) |
| MD / PicoDrive | ✅ launches and plays; former Auto CPU slowdown near 480 MHz is fixed (user-verified 2026-07-20) |
| FBN / FBNeo | ⚠️ partial — with required BIOS, launch and play are OK (user-verified 2026-07-20). Missing-BIOS path still hard-locks MinArch (no menu/exit). **beta1:** ship as known issue (most users on happy path with BIOS). **RC gate:** must fix error recovery before release candidate |
| PS / PCSX-ReARMed | ✅ launches and plays after a clean core rebuild; the earlier crashes were caused by a stale/corrupted core, not an H700 runtime defect |
| Remaining shipped cores | ⚠️ expanded 2026-07-20: A2600, MGBA, SMS pass in addition to GB/GBC/GBA/FC/SFC/MD/PS. Full EXTRAS long-tail still pending (see 10) |

### Beta1 vs RC gate summary

The matrix does not require every long-tail row to turn green before beta1.

**beta1 may ship with:**
- FBNeo missing-BIOS hard-lock as a **known issue** (happy path with BIOS is fine;
  most users will not hit it). Document in release notes.
- SIGUSR1 shutdown and dirty-card `fsck` recovery still untested (RC polish).
- BT audio lifecycle incomplete where device/firmware scan is broken.
- Overnight drain partial (voltage/stock re-test when convenient).
- Remaining EXTRAS cores untested beyond the green rows in 10.

**RC must close (or explicitly re-descope):**
- **FBNeo missing-BIOS recovery** — major bug; fix so MinArch returns control
  without a power cycle. Preserve the case as a regression test afterward.
- SIGUSR1 end-to-end + dirty-card recovery smoke (RC polish gauntlet).
- Prefer BT lifecycle revalidation once device BT scan works again.
- Prefer overnight drain voltage-vs-stock pass.

**Closed / not required as test rows:**
- Clean uninstall — design-guaranteed by deleting `dmenu.bin` (removed from matrix).
- MU-style theme — install prerequisite only.
- Closed-lid POWER, charging→light-sleep, headphone hardware route — accepted policy.
- MD Auto CPU — fixed.

RGcubexx full hardware coverage, HDMI boot/EDID/model expansion, cross-platform
controller normalization, Pak Store/OTA, per-panel calibration, H700-sized overlay
content packs, and the dedicated toolchain image remain post-beta / optional unless
scope is promoted.

### RG34XXSP remaining validation

- To claim full parity rather than alpha coverage, repeat the RG40XXV-scoped rows on
  RG34XXSP: cold-boot soak, fresh-install defaults, brightness/colortemp behavior,
  the full shader/effect sweep, rumble, and WiFi scan/connect/forget/sleep/NTP.
- Run the full per-core protocol in [10](10-core-game-matrix.md), including menu,
  clean exit, save/load state, sustained Auto CPU play, sleep/resume, and power-off
  auto-resume. The initial green results establish launch/gameplay, not every one of
  those checks for every system.
- Screenshots pass on RG34XXSP. UI scaling good at 720×480. Panel-matched overlay
  assets are not shipped for H700 (only Brick-sized GBA PNGs exist).
- FBNeo with BIOS: pass. Missing-BIOS hard-lock: **known issue for beta1**; **RC
  must fix**, then preserve as a regression test.
- Preserve PS1 in the release-candidate smoke pass so a stale or corrupted packaged
  core cannot recreate the resolved launch failure.
- ~~Reproduce the MD Auto CPU behavior~~ Fixed (user-verified 2026-07-20).
- ~~Test 720×480 Bootlogo backup and restore~~ Pass on RG34XXSP (2026-07-20).
- Test BT controller pairing/input when device BT scan works. Overnight drain
  partial on RG34XXSP (~7% over 8.5 h); re-test with voltage_now vs stock (RC
  polish). Battery % vs stock passes; icon-vs-Battery-pak desync is a follow-up.
- ~~Clean uninstall~~ removed from matrix (delete `dmenu.bin` is sufficient by
  design). SIGUSR1 + dirty-card recovery: **RC polish**, untested OK for beta1.
  MU-style theme is a documented stock-side install prerequisite only.

### Robustness
| Item | Status |
|---|---|
| SIGUSR1 from `launcher.sh stop` doesn't corrupt | ⬜ RC polish — shim traps USR1; end-to-end untested; not a beta1 gate |
| SD dirty-card recovery (`fsck.fat -a` hook in boot shim) | ⬜ RC polish — code in place; not a beta1 gate |
| Stock “MU style” theme vs NextUI hijack | ➖ closed — not a NextUI runtime test. Selecting stock/stockmod **MU style 1/2** makes `dmenu_ln` prefer the MU frontend over `/mnt/mmc/dmenu.bin`, so NextUI never starts and no “STOCK TARGET REQUIRED” splash can appear. Confirmed: device just boots stock/MU UI. Nothing the shim can do from outside that path; install docs already require **old style** theme (`skeleton/BASE/README.txt`). The boot-shim `muos1.ini`/`muos2.ini` warning is effectively unreachable for the same reason (override happens before our `dmenu.bin` runs) |

## Regression guardrails

- **Every h700 PR must list its `workspace/all/` touches and re-run
  `make PLATFORM=tg5040 build`** (both platforms build from the same shared tree).
  The branch's complete list of net shared-code touches, for reference:
  1. `workspace/all/audiomon/audiomon.cpp` — `bluetoothAudioAvailable()` guard (skip A2DP without bluealsa) plus an H700-only compile guard that omits the unsupported `delay 0` line (+Makefile dep fix)
  2. `workspace/all/common/generic_video.c` — hard `exit(1)` on SDL/GL init failures
  3. `workspace/all/minarch/makefile` — h700 added to tg5040-class feature filters (RA/CHD/SRM/samplerate)
  4. `workspace/all/nextui/nextui.c` — startup LOG_info breadcrumbs
  5. `workspace/all/settings/btmenu.cpp/.hpp` — `rateItem` made optional (`NO_BT_AUDIO`)
  6. `workspace/all/settings/makefile` — h700 block (`-DHAS_BTAGENT`, btagent, glib; the temporary `NO_BT_AUDIO` gate was removed after stock BlueALSA validation)
  7. `workspace/all/settings/settings.cpp` — Anbernic vendor/models, h700 platform + capability flags
  8. `workspace/all/syncsettings/syncsettings.c` — also restore colortemp/contrast/saturation/exposure/displaycal on resume
  9. `workspace/makefile` — rfkill for h700; ledcontrol gated to tg50x0, bootlogo to tg50x0+h700
  10. Root `makefile` + `makefile.toolchain` — h700 platform + tg5040-image reuse
  11. `workspace/all/common/api.c`/`api.h` — `PWR_requestSleep()` (lid sleep) and
  `SND_quit()` in `PWR_enterSleep` (close ALSA before suspend; see 06). Both are
  platform-agnostic changes — watch them when rebasing onto main.
  12. `workspace/all/bootlogo/bootlogo.c` — load/path/count diagnostics, on-screen
  empty-state, empty-list apply guard, and the optional
  `BOOTLOGO_PREVIEW_ROTATE_CW` preview-rotation hook (defaults off; only h700's
  platform.h defines it, for the RG28XX).
- **New-button / platform-API stubs must land on every platform.** The tg5050 L4/R4
  work and later main features (`BTN_FN*` #788, `PLAT_isUSBConnected` #783) break
  h700 until matching defines/stubs exist. When main grows a button or `PLAT_*`
  hook, grep every `platform/platform.h` / `platform.c` — and verify with a full
  `make PLATFORM=h700` (single-pak builds miss some translation units).
- **Re-run the 00-device-facts probes after each Anbernic stock-firmware update.**
  Paths have been stable historically, but `dmenu_ln`/muOS hooks are
  stockmod-version-dependent, and the model-string detector reads a stock binary.
- When adding or promoting a device (34XXSP/28XX/cube): walk this matrix top to
  bottom on that device; the ⬜ rows above are the backlog for RG40XXV too.
  (RG34XXSP hallkey semantics are settled: 1 = lid open, polled from sysfs, no evdev
  switch exists, and nothing else in the stock/stockmod stack reacts to it — 06.)
