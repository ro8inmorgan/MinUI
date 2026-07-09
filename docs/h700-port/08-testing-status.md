# 08 — Testing Status & Regression Guardrails

## Test fleet

| Device | Evidence | Role |
|---|---|---|
| RG40XXV | Probed live | Primary bring-up device |
| RG34XXSP | User-tested | General parity with RG40XXV; lid wake/sleep broken |
| RG28XX | Not yet tested | Rotation validation target |
| TrimUI Brick TG5040 | Probed live | Regression reference (behavioral 1:1 comparisons) |

**Dev-loop tips:**
- The stock OS runs sshd with a full Ubuntu userland — iterate by `scp`ing freshly
  built `.elf`s into `.system/h700/bin` and restarting the launch loop. No SD-card
  shuffling after first install.
- Drop the `debug-keep-network` flag file (see launch.sh) to keep WiFi+SSH up during
  testing; 5 consecutive nextui crashes also auto-start SSH for 300 s.

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
| Shaders (all shipped .glsl), overlays, effects on Mali-G31 | ✅ |
| Volume 0–20 + mute | ✅ |
| Rumble (moto on/off) | ✅ |
| WiFi scan/connect/forget | ✅ |
| WiFi survives sleep; NTP sync | ✅ |
| **Deep sleep (auto/manual/in-game) + wake** | ⚠️ **wake unreliable — sometimes hangs entering/leaving suspend (see 06)** |
| Screenshots, Recently Played, game switcher, box art | ⬜ |
| Clean uninstall (delete dmenu.bin → pristine stock) | ⬜ |
| Battery % accuracy vs stock; charging indicator; charge-while-sleeping | ⬜ |
| Overnight drain (8 h mem-sleep vs stock baseline) | ⬜ (blocked on wake reliability) |
| RetroAchievements login + unlock | ⬜ |
| Pak Store install | ⬜ |
| OTA update flow | ⬜ |
| BT controller pairing + input | ⬜ |
| BT A2DP audio | ✖ gated off (`NO_BT_AUDIO`, no bluealsa shipped — 05/07) |
| Headphone jack detection | ✖ not wired (05) |
| RG34XXSP: general H700 port + 720×480 UI | ✅ user-tested; works like RG40XXV |
| RG34XXSP: lid sleep/wake | ⚠️ tested, broken — lid does not sleep/wake; screen stays on |
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
  9. `workspace/makefile` — rfkill for h700; ledcontrol/bootlogo gated to tg50x0
  10. Root `makefile` + `makefile.toolchain` — h700 platform + tg5040-image reuse
  (`workspace/all/common/api.c` was touched by the alpha-blit experiment and fully
  reverted — net zero.)
- **Re-run the 00-device-facts probes after each Anbernic stock-firmware update.**
  Paths have been stable historically, but `dmenu_ln`/muOS hooks are
  stockmod-version-dependent, and the model-string detector reads a stock binary.
- When adding or promoting a device (34XXSP/28XX/cube): walk this matrix top to
  bottom on that device; the ⬜ rows above are the backlog for RG40XXV too. For
  RG34XXSP specifically, the next targeted probes are `hallkey` polarity/values and
  whether the kernel/stock stack handles any lid action before NextUI sees it.
