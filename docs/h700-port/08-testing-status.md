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

## Validation matrix (status as of 2026-07-17, RG40XXV unless noted)

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
| Deep sleep (auto/manual) + wake | ⚠️ core paths work on RG34XXSP and RG40XXV, including in-game resume and power-off auto-resume. RG34XXSP POWER can wake the device while the lid is closed; charging intentionally prevents deep sleep in shared code (06) |
| Files app (NextCommander): input, scaling, first-frame render | ✅ tested on RG40XXV and RG34XXSP; PPU 2 retained for alpha |
| Input pak: per-device stick layout | ✅ RG40XXV shows working L3 only; RG34XXSP shows working L3/R3. Analog movement is not visualized by the existing Input pak and is out of scope for this port |
| Recently Played + game switcher | ✅ tested on RG40XXV and RG34XXSP |
| Screenshots | ⬜ testing planned |
| Box art | ✅ tested at 720×480 on RG34XXSP |
| HDMI output | ⚠️ RG40XXV + TV passes menu/game output and in-game hotplug in both directions, including audio routing. Boot-with-cable, other H700 models, and displays that reject the fixed 1080p60 mode remain untested; HDMI is a stretch feature rather than a beta gate (04) |
| Clean uninstall (delete dmenu.bin → pristine stock) | ⬜ |
| Battery % accuracy vs stock | ⬜ |
| Charging detection/indicator; sleep while charging | ⚠️ indicator works on RG34XXSP; charging permits light sleep but shared power code deliberately suppresses deep sleep |
| Overnight drain (8 h mem-sleep vs stock baseline) | ⬜ (unblocked — sleep/wake now reliable) |
| RetroAchievements login + unlock | ✅ works on RG34XXSP; on-screen credential keyboard also verified |
| Pak Store install | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| OTA update flow | ➖ explicitly excluded from alpha scope; testing is not applicable for this release |
| BT controller semantic mapping | ⚠ pairing and input attachment pass; raw button indices vary by controller and require a later cross-platform tg5040/tg5050/H700 normalization change |
| Bootlogo pak, RG40XXV 640×480 | ✅ carousel/apply/backup tested; restore inferred via same apply path |
| Bootlogo pak, RG34XXSP 720×480 | ✅ fixed 2026-07-13 — all 23 presets load and render with the current build (verified via SSH-driven run + framebuffer capture; the failing binary was alpha1.1-era). Pak now logs path/count/load errors and shows the searched path on screen when empty; apply guarded against an empty list |
| Bootlogo pak, RG28XX 480×640 | ✅ apply user-tested: logo renders upright at boot. Previews were shown panel-native (90° off) and are now rotated to boot orientation (`BOOTLOGO_PREVIEW_ROTATE_CW`) |
| BT A2DP audio | ⚠ AirPods 4 ANC pair/connect and user-audible SBC game audio pass on RG40XXV; internal speaker mutes/restores correctly. Confirmed blockers were the H700 stock plugin rejecting `delay 0` and BlueZ transport volume initializing to zero—not 44.1/48 kHz. H700 now uses only stock BlueALSA with native volume; final automatic reconnect/game-switch/suspend-resume tests remain (05/07) |
| BT maximum sampling rate | ⚠ Matches tg5040 and defaults to 48000 Hz. AirPods accepted both 44100 and 48000 during probing, so the option is a compatibility escape hatch rather than a fix for the observed silence. Verify the rebuilt Settings UI changes and persists the value. The inherited connection test also sees controller-only ACL links; at the 48000 default this is harmless, while a future audio-specific detector is a cross-platform cleanup rather than an H700 beta blocker |
| Headphone jack detection | ✖ not wired (05) |
| RG34XXSP: general H700 port + 720×480 UI | ✅ Battery, Game Tracker, Input, Clock, Settings, Files, keyboard, box art, game switcher and in-game menus work well; resolution-specific overlays untested |
| RG34XXSP: lid sleep/wake | ⚠️ lid close and open work; deep suspend requires power as expected, but power also wakes light sleep while the lid is closed |
| RG28XX: rotated UI + games | ✅ user-tested — UI correct; minarch Aspect/Fullscreen were broken by app-side double-rotation, fixed by removing `should_rotate` (rotation is driver-level only — 04) |
| RGcubexx: 720×720 | ⬜ no device available; intentionally an external alpha-validation target |

### Performance
| Item | Status |
|---|---|
| GBA/SNES full speed with vsync, no audio underruns | ✅ on RG40XXV and in tested RG34XXSP games |
| PS1 launch/performance | ✅ on RG40XXV and RG34XXSP. The apparent RG34XXSP launch regression was a stale/corrupted core artifact; a clean core rebuild fixed it |
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
| PS / PCSX-ReARMed | ✅ launches and plays after a clean core rebuild; the earlier crashes were caused by a stale/corrupted core, not an H700 runtime defect |
| Remaining shipped cores | ⬜ systematic coverage pending |

### Beta-entry gate summary

The matrix does not require every long-tail row to turn green before beta. The beta
entry gates are the known user-facing regressions and the safety/lifecycle checks:

- **Core regressions:** fix the FBNeo missing-BIOS lockup; either fix MD Auto CPU
  slowdown or ship a proven safe default. Run at least
  a launch/audio/input/menu/exit/save-state smoke test across the remaining shipped
  core families.
- **Bluetooth audio lifecycle:** verify automatic reconnect, game-to-game and
  menu-to-game switching, five suspend/resume cycles, case/disconnect speaker restore,
  and Settings sample-rate selection/persistence. Controller mapping and the
  controller-only sample-rate detector are shared-platform follow-ups, not H700 gates.
- **Power and reversibility:** complete battery-vs-stock accuracy and overnight-drain
  measurements, decide the closed-lid POWER and charging/light-sleep policies, and
  pass clean uninstall, SIGUSR1 shutdown, and dirty-card recovery.
- **Boot policy and basic I/O:** decide whether muOS/stockmod detection warns or
  hard-stops, test screenshots and resolution-specific overlays, and determine whether
  the headphone jack is hardware-auto-switched or needs software handling.

RGcubexx/720×720 hardware coverage, HDMI boot/EDID/model expansion, cross-platform
controller normalization, Pak Store/OTA, per-panel calibration, and the dedicated
toolchain image remain explicit beta follow-ups unless their scope is promoted.

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
- Preserve PS1 in the release-candidate smoke pass so a stale or corrupted packaged
  core cannot recreate the resolved launch failure.
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
