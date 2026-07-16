# 08 — Testing Status & Regression Guardrails

## Test fleet

| Device | Evidence | Role |
|---|---|---|
| RG40XXV | Probed live | Primary bring-up device |
| RG34XXSP | User-tested | General parity with RG40XXV; sleep/wake + lid validated here |
| RG28XX | User-tested | Rotation validated: UI, game scaling, bootlogo apply working |
| RGcubexx | No device available | External alpha-validation target |
| TrimUI Brick TG5040 | Probed live | Regression reference (behavioral 1:1 comparisons) |

**Dev-loop tips:**
- The stock OS runs sshd with a full Ubuntu userland — iterate by `scp`ing freshly
  built `.elf`s into `.system/h700/bin` and restarting the launch loop. No SD-card
  shuffling after first install.
- Repeated `nextui.elf` crashes now log the crash limit and power off. Use TF2 logs
  for crash-loop debugging; the release runtime powers off instead of starting SSH.

## Validation matrix (status as of 2026-07-13, RG40XXV unless noted)

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
| Shaders (all shipped .glsl), overlays, effects on Mali-G31 | ✅ |
| Volume UI + levels, mute through 100% | ✅ tested on RG40XXV and RG34XXSP; no known issues |
| Rumble (moto on/off) | ✅ |
| WiFi scan/connect/forget | ✅ |
| WiFi survives sleep; NTP sync | ✅ |
| BT adapter init + controller pairing/attachment | ✅ RG40XXV: vendor UART attach creates `hci0`; stock BlueZ 5.66 discovers, pairs, trusts, and connects a DualSense; NextUI opens it as an SDL joystick plus `/dev/input/event3`; five restart cycles passed without duplicate attach/daemon processes. Raw button semantics remain a platform-agnostic tg5040/tg5050/H700 mapping issue and are out of scope for this branch |
| Deep sleep (auto/manual) + wake | ⚠️ core paths work on RG34XXSP and RG40XXV, including in-game resume and power-off auto-resume. RG34XXSP POWER can wake the device while the lid is closed; charging intentionally prevents deep sleep in shared code (06) |
| Files app (NextCommander): input, scaling, first-frame render | ✅ tested on RG40XXV and RG34XXSP; PPU 2 retained for alpha |
| Input pak: per-device stick layout | ✅ RG40XXV shows working L3 only; RG34XXSP shows working L3/R3. Analog movement is not visualized by the existing Input pak and is out of scope for this port |
| Recently Played + game switcher | ✅ tested on RG40XXV and RG34XXSP |
| Screenshots | ⬜ testing planned |
| Box art | ✅ tested at 720×480 on RG34XXSP |
| Clean uninstall (delete dmenu.bin → pristine stock) | ⬜ |
| Battery % accuracy vs stock | ⬜ |
| Charging detection/indicator; sleep while charging | ⚠️ indicator works on RG34XXSP; charging permits light sleep but shared power code deliberately suppresses deep sleep |
| Overnight drain (8 h mem-sleep vs stock baseline) | ⬜ (unblocked — sleep/wake now reliable) |
| RetroAchievements login + unlock | ✅ works on RG34XXSP; on-screen credential keyboard also verified |
| Pak Store install | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| OTA update flow | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| BT controller pairing + input | ⬜ |
| Bootlogo pak, RG40XXV 640×480 | ✅ carousel/apply/backup tested; restore inferred via same apply path |
| Bootlogo pak, RG34XXSP 720×480 | ✅ fixed 2026-07-13 — all 23 presets load and render with the current build (verified via SSH-driven run + framebuffer capture; the failing binary was alpha1.1-era). Pak now logs path/count/load errors and shows the searched path on screen when empty; apply guarded against an empty list |
| Bootlogo pak, RG28XX 480×640 | ✅ apply user-tested: logo renders upright at boot. Previews were shown panel-native (90° off) and are now rotated to boot orientation (`BOOTLOGO_PREVIEW_ROTATE_CW`) |
| BT A2DP audio | ✖ gated off (`NO_BT_AUDIO`, no bluealsa shipped — 05/07) |
| Headphone jack detection | ✖ not wired (05) |
| RG34XXSP: general H700 port + 720×480 UI | ✅ Battery, Game Tracker, Input, Clock, Settings, Files, keyboard, box art, game switcher and in-game menus work well; resolution-specific overlays untested |
| RG34XXSP: lid sleep/wake | ⚠️ lid close and open work; deep suspend requires power as expected, but power also wakes light sleep while the lid is closed |
| RG28XX: rotated UI + games | ✅ user-tested — UI correct; minarch Aspect/Fullscreen were broken by app-side double-rotation, fixed by removing `should_rotate` (rotation is driver-level only — 04) |
| RGcubexx: 720×720 | ⬜ no device available; intentionally an external alpha-validation target |

### Performance
| Item | Status |
|---|---|
| GBA/SNES/PS1 full speed with vsync, no audio underruns | ✅ on RG40XXV; RG34XXSP PS1 currently crashes during launch |
| Frame pacing / tearing / input lag vs stock RA | ✅ gameplay-tested on RG40XXV; no visible pacing, tearing, latency, or audio issues (not instrumented) |
| Manual governor changes don't stutter audio | ✅ tested in-game on RG34XXSP |
| Auto governor load/frequency selection | ⚠️ MD can remain near 480 MHz and slow down depending on shader/scaling; stock shader + 3× + linear raises ~720 MHz and is smooth. Powersave/performance are smooth; needs debugging |

### RG34XXSP core/game bring-up

Detailed results and the remaining system backlog are tracked in
[10-core-game-matrix.md](10-core-game-matrix.md).

| System/core | Status |
|---|---|
| GB / GBC / GBA / FC / SFC | ✅ games launch and play perfectly |
| MD / PicoDrive | ⚠️ launches, but some rendering combinations slow down with auto CPU near 480 MHz |
| FBN / FBNeo | ⚠️ missing BIOS blocked game validation; the error path then left MinArch unable to open its menu or exit |
| PS / PCSX-ReARMed | ⚠️ crashes back to NextUI during launch for every tested game |
| Remaining shipped cores | ⬜ systematic coverage pending |

### RG34XXSP remaining validation

- To claim full parity rather than alpha coverage, repeat the RG40XXV-scoped rows on
  RG34XXSP: cold-boot soak, fresh-install defaults, brightness/colortemp behavior,
  the full shader/effect sweep, rumble, and WiFi scan/connect/forget/sleep/NTP.
- Run the full per-core protocol in [10](10-core-game-matrix.md), including menu,
  clean exit, save/load state, sustained Auto CPU play, sleep/resume, and power-off
  auto-resume. The initial green results establish launch/gameplay, not every one of
  those checks for every system.
- Test screenshots and overlays made for 720×480.
- Retest FBNeo with the required BIOS, and separately preserve the missing-BIOS error
  case as a regression test after it is fixed.
- Diagnose PS1 launch and retest multiple formats/titles with the required BIOS.
- Reproduce the MD Auto CPU behavior with exact shader, scale, and interpolation
  combinations.
- Test 720×480 Bootlogo backup and restore (previews and apply now work; the
  first-apply `original.bmp` backup and restoring it haven't been exercised on SP).
- Test BT controller pairing/input, battery percentage accuracy, and overnight drain.
- Run clean uninstall, SIGUSR1 shutdown, dirty-card recovery, and muOS/stockmod
  coexistence on this hardware where practical.

### Robustness
| Item | Status |
|---|---|
| SIGUSR1 from `launcher.sh stop` doesn't corrupt | ⬜ (shim traps USR1; end-to-end untested) |
| SD dirty-card recovery (`fsck.fat -a` hook in boot shim) | ⬜ (code in place) |
| muOS/stockmod coexistence: detected + splash warning | ⬜ (code in place; warn-only by design so far) |

## Regression guardrails

- **Every h700 PR must list its `workspace/all/` touches and re-run
  `make PLATFORM=tg5040 build`** (both platforms build from the same shared tree).
  The branch's complete list of net shared-code touches, for reference:
  1. `workspace/all/audiomon/audiomon.cpp` — `bluetoothAudioAvailable()` guard (skip A2DP without bluealsa) (+Makefile dep fix)
  2. `workspace/all/common/generic_video.c` — hard `exit(1)` on SDL/GL init failures
  3. `workspace/all/minarch/makefile` — h700 added to tg5040-class feature filters (RA/CHD/SRM/samplerate)
  4. `workspace/all/nextui/nextui.c` — startup LOG_info breadcrumbs
  5. `workspace/all/settings/btmenu.cpp/.hpp` — `rateItem` made optional (`NO_BT_AUDIO`)
  6. `workspace/all/settings/makefile` — h700 block (`-DHAS_BTAGENT -DNO_BT_AUDIO`, btagent, glib)
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
- **New-button defines must land in every platform.h.** The tg5050 L4/R4 work added
  `CODE_L4/R4` + `JOY_L4/R4` references to shared `api.c` and `BUTTON_L4/R4` to
  `minput.c`; h700 didn't build until the `*_NA` defines were added (`d738014`,
  `956ed34`). When main grows a button, grep every `platform/platform.h` for the
  new `BUTTON_/CODE_/JOY_` names — and verify with a full `make PLATFORM=h700`
  (single-pak builds don't compile `minput.c` and will miss `BUTTON_*` gaps).
- **Re-run the 00-device-facts probes after each Anbernic stock-firmware update.**
  Paths have been stable historically, but `dmenu_ln`/muOS hooks are
  stockmod-version-dependent, and the model-string detector reads a stock binary.
- When adding or promoting a device (34XXSP/28XX/cube): walk this matrix top to
  bottom on that device; the ⬜ rows above are the backlog for RG40XXV too.
  (RG34XXSP hallkey semantics are settled: 1 = lid open, polled from sysfs, no evdev
  switch exists, and nothing else in the stock/stockmod stack reacts to it — 06.)
