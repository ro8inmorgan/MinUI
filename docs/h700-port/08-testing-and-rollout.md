# 08 — Testing, Validation & Rollout

## Test fleet
- RG40XXV (192.168.34.55, root/root, stockmod) — primary bring-up device
- RG34XXSP — second target (lid, 720×480)
- RG28XX — phase-2 target (rotation)
- TrimUI Brick TG5040 (192.168.34.81, root/tina) — regression reference (behavioral 1:1 comparisons)

Dev-loop tip: the stock OS runs sshd out of the box and has a full Ubuntu userland —
iterate by `scp`ing freshly built `.elf`s directly into `.system/h700/bin` and
restarting the launch loop; no SD-card shuffling needed after first install.

## Phase 0 exit criteria (feasibility spikes — do these FIRST)
| # | Spike | Pass criteria |
|---|---|---|
| 0.1 | ~~Cross hello-world~~ **DONE ✔** | tg5040-built displaycal.elf runs on RG40XXV stockmod + RG34XXSP Knulli |
| 0.2 | ~~ELF class + version of Mali blob~~ **DONE ✔** | 64-bit aarch64, ES 3.2 r20p0, fbdev winsys (probed live) |
| 0.3 | Raw EGL/GLES fbdev probe | context created from *our* binary; colored clear visible on panel (blob capabilities already confirmed; this tests the winsys handshake) |
| 0.4 | Custom SDL2 (malifbdev) + GL context | SDL window + GL swap at 60fps, `SDL_GetCurrentDisplayMode` sane |
| 0.5 | disp ioctls | brightness 0x102 works; gamma LUT 0x10b/0x10c visibly tints screen; survives suspend? |
| 0.6 | Suspend/resume harness | echo mem → power-button wake → script continues; repeat 20× in loop without hang |
| 0.7 | Input dump | evtest maps every button/stick on RG40XXV; SDL joystick sees same |
| 0.8 | ALSA beep | aplay a wav through SDL-ALSA path at 48k |

Any 0.x failure = re-plan before writing platform code (each has a documented fallback
in docs 01/04).

## Per-feature validation matrix (parity with Brick = the bar)

Functional (each device, each release):
- [ ] cold boot to NextUI < stock-boot + 5s; boot loop stable across 10 reboots
- [ ] browse/launch/quit game in every SYSTEM pak core; save/load state; in-game menu
- [ ] fast resume (BTN_RESUME from launcher), auto-resume after power-loss
- [ ] brightness 0-10 + colortemp ramps; settings persist across reboot
- [ ] volume 0-20, mute, (headphones if jack detection lands)
- [ ] displaycal: RGB gain sliders visibly act; persist; survive sleep + game launch
- [ ] shaders: each shipped .glsl loads on Mali-G31; overlays; effects (scanline/grid)
- [ ] screenshots (PLAT_captureRendererToSurface), Recently Played, game switcher, box art
- [ ] deep sleep: auto (idle), manual (power key), in-game sleep; wake resumes exactly; 20-cycle soak
- [ ] overnight drain ≤ stock + 1% (8h mem-sleep; measure stock baseline same unit first)
- [ ] battery % accuracy vs stock reading; charging indicator; charge-while-sleeping
- [ ] WiFi: scan/connect/forget, survives sleep, NTP sync, RetroAchievements login+unlock, Pak Store install, OTA update flow
- [ ] BT: pair controller (input in launcher + game), A2DP audio with samplerate limit
- [ ] rumble: minarch rumble test core / game (moto on/off)
- [ ] RG34XXSP: lid close → sleep; lid open → wake; power key ignored while closed
- [ ] RG28XX: whole UI + games correctly rotated, no perf penalty (measure fps)
- [ ] uninstall: delete dmenu.bin → pristine stock boot

Performance (compare against same game/core on Brick, correcting for resolution):
- [ ] GBA/SNES/PS1 full speed with vsync, no audio underruns (SAMPLES tuning)
- [ ] frame pacing: no tearing (fbdev double buffering / vsync via mali backend), input lag comparable to stock RA
- [ ] CPU governor transitions don't stutter audio

Robustness:
- [ ] SIGUSR1 from launcher.sh stop doesn't corrupt
- [ ] SD yank / readonly card recovery path (MinUI.pak launch.sh has e2fsck hook — vfat equivalent: fsck.fat)
- [ ] stockmod coexistence: muos1.ini present → document + detect + warn in installer output

## Rollout phases

| Phase | Deliverable | Est. effort (1 experienced dev) |
|---|---|---|
| 0 | Feasibility spikes 0.1–0.8 on RG40XXV (0.1/0.2/0.3-version/0.6 already done) | 2–3 days |
| 1 | Build wiring: `make PLATFORM=h700 shell` via tg5040 image + in-tree SDL2 `early` build | 1–2 days |
| 2 | Boot shim + installer + skeleton; device boots to a stub | 3–5 days |
| 3 | Platform layer: video/input/audio/msettings/keymon → **nextui.elf usable** | 1–2 weeks |
| 4 | minarch + full core set + per-emu paks | 1 week |
| 5 | Power/sleep/battery/lid + rumble + governor | 1 week |
| 6 | WiFi + BT + displaycal + timezones/NTP | 1 week |
| 7 | RG34XXSP bring-up (mostly config: 720×480 + lid) | 2–3 days |
| 8 | Beta hardening on 40XXV+34XXSP, full matrix above | 1–2 weeks |
| 9 | RG28XX rotation | 3 days–1.5 weeks (depends on SDL rot path) |
| 10 | (later) RGcubexx, HDMI out, RG35XX family variants, Panel-Fix tool port | — |

Total to a solid 40XXV+34XXSP beta: **~6–8 weeks** single-dev, parallelizable to
~3–4 weeks with two devs (one on 0–2 infra, one on 3–6 platform code).

## Regression guardrails
- Keep `h700` additions out of `workspace/all/` except: rotation hook (04), any
  `HAS_BTAGENT`-style makefile gates, updater case — list every `all/` touch in the PR
  description and re-run a tg5040 build (`make PLATFORM=tg5040 build`) in CI for every
  h700 PR (both platforms build from the same shared tree).
- Snapshot the probe outputs (00-device-facts) into the repo; re-run the probe script
  after each Anbernic stock-firmware update (paths have historically been stable, but
  `dmenu_ln`/muos hooks are stockmod-version-dependent).
