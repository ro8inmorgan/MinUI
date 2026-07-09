# 08 — Testing Status & Regression Guardrails

## Test fleet

| Device | Evidence | Role |
|---|---|---|
| RG40XXV | Probed live | Primary bring-up device |
| RG34XXSP | User-tested | General parity with RG40XXV; sleep/wake + lid validated here |
| RG28XX | Not yet tested | Rotation validation target |
| TrimUI Brick TG5040 | Probed live | Regression reference (behavioral 1:1 comparisons) |

**Dev-loop tips:**
- The stock OS runs sshd with a full Ubuntu userland — iterate by `scp`ing freshly
  built `.elf`s into `.system/h700/bin` and restarting the launch loop. No SD-card
  shuffling after first install.
- Repeated `nextui.elf` crashes now log the crash limit and power off. Use TF2 logs
  for crash-loop debugging; the release runtime powers off instead of starting SSH.

## Validation matrix (status as of 2026-07-09, RG40XXV unless noted)

✅ tested & passed ⚠️ tested, has issues ⬜ untested ✖ not implemented

### Functional
| Item | Status |
|---|---|
| Cold boot to NextUI, stable across 10+ reboots, reasonable boot time | ✅ |
| Browse/launch/quit games across system pak cores; save/load state; in-game menu | ✅ |
| Fast resume (BTN_RESUME) + auto-resume after power loss | ✅ |
| Brightness 0–10 + colortemp ramps; persist across reboot | ✅ |
| MENU short-tap → shortcuts overlay vs. hold → brightness (compound-tap fix, see 00/03) | ✅ tested on RG34XXSP; firmware quirk is common to the H700 line, expected fine on all RG XX |
| Displaycal RGB gains: act, persist, survive sleep + game launch | ✅ |
| Displaycal survives reboot (was clobbered to defaults every boot; fixed 2026-07-09) | ✅ verified on RG40XXV by patching msettings.bin + reboot |
| Fresh-install defaults: brightness 4, displaycal off/neutral | ✅ verified on RG40XXV (deleted msettings.bin + reboot) |
| Dead enhance controls (contrast/saturation/exposure) hidden on h700 | ✅ code-gated 2026-07-09; visual check on device pending |
| Shaders (all shipped .glsl), overlays, effects on Mali-G31 | ✅ |
| Volume UI + levels, mute through 100% | ✅ tested on RG40XXV and RG34XXSP; no known issues |
| Rumble (moto on/off) | ✅ |
| WiFi scan/connect/forget | ✅ |
| WiFi survives sleep; NTP sync | ✅ |
| Deep sleep (auto/manual) + wake | ✅ fixed & user-tested on RG34XXSP 2026-07-09 (4 stacked bugs — see 06); in-game (minarch) path shares the code but untested; RG40XXV re-test pending |
| Files app (NextCommander): input, scaling, first-frame render | ✅ fixed & user-tested on RG40XXV 2026-07-09 (SDL joystick classification patch, button index remap, square-window + oversize-window fixes, startup warmup render, PPU 2 — see 01/03) |
| Input pak: per-device stick layout (L3/R3 pills only where sticks exist; RG40XXV shows L3 only) | ⬜ (code-gated 2026-07-10 via `dev_has_lstick/rstick` — see 03; needs on-device check + `RGXX_MODEL` string capture on RG35xx family / RG40xxH) |
| Screenshots, Recently Played, game switcher, box art | ⬜ |
| Clean uninstall (delete dmenu.bin → pristine stock) | ⬜ |
| Battery % accuracy vs stock; charging indicator; charge-while-sleeping | ⬜ |
| Overnight drain (8 h mem-sleep vs stock baseline) | ⬜ (unblocked — sleep/wake now reliable) |
| RetroAchievements login + unlock | ⬜ |
| Pak Store install | ⬜ |
| OTA update flow | ⬜ |
| BT controller pairing + input | ⬜ |
| Bootlogo pak: preset carousel, apply (writes `mmcblk0p2`), first-apply `original.bmp` backup, restore | ✅ user-tested on RG40XXV 2026-07-09: apply works, `original.bmp` backup captured; reboot after apply is a bit slow but acceptable |
| BT A2DP audio | ✖ gated off (`NO_BT_AUDIO`, no bluealsa shipped — 05/07) |
| Headphone jack detection | ✖ not wired (05) |
| RG34XXSP: general H700 port + 720×480 UI | ✅ user-tested; works like RG40XXV |
| RG34XXSP: lid sleep/wake | ✅ lid close sleeps; lid open wakes from screen-off; power key wakes from deep suspend (stock-like, desired — see 06) |
| RG28XX: rotated UI + games | ⬜ (plumbing in place, unvalidated — 04) |
| RGcubexx: 720×720 | ⬜ |

### Performance
| Item | Status |
|---|---|
| GBA/SNES/PS1 full speed with vsync, no audio underruns | ✅ |
| Frame pacing / tearing / input lag vs stock RA | ⬜ (no complaints observed, not measured) |
| Governor transitions don't stutter audio | ⬜ |

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
- **Re-run the 00-device-facts probes after each Anbernic stock-firmware update.**
  Paths have been stable historically, but `dmenu_ln`/muOS hooks are
  stockmod-version-dependent, and the model-string detector reads a stock binary.
- When adding or promoting a device (34XXSP/28XX/cube): walk this matrix top to
  bottom on that device; the ⬜ rows above are the backlog for RG40XXV too.
  (RG34XXSP hallkey semantics are settled: 1 = lid open, polled from sysfs, no evdev
  switch exists, and nothing else in the stock/stockmod stack reacts to it — 06.)
