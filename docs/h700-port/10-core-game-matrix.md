# 10 — Core & Game Compatibility Matrix

This is the system-by-system hardware validation ledger for the H700 port. The first
pass is on RG34XXSP (720×480); add RG40XXV and other RG XX results where behavior
differs.

The existing results predate the full protocol below. A green initial result means
launch and gameplay were reported good; it does not yet imply every protocol step was
repeated for that system.

✅ passed ⚠️ launches or reaches the core but has an issue ⬜ untested

## Minimum test per system

1. Launch at least one known-good ROM with required BIOS files present.
2. Verify video, audio, controls, in-game menu, clean exit, save and load state.
3. Play for at least 10 minutes on Auto CPU and note frequency/performance anomalies.
4. Exercise sleep/resume and power-off auto-resume for representative major systems.
5. Record the exact game, format, shader, scale, interpolation, CPU profile, and BIOS
   state for every failure.

## RG34XXSP results (2026-07-10)

| System | Core | Status | Evidence / issue |
|---|---|---|---|
| GB | Gambatte | ✅ | Launch and gameplay perfect |
| GBC | Gambatte | ✅ | Launch and gameplay perfect |
| GBA | gpSP | ✅ | Launch and gameplay perfect |
| FC | FCEUmm | ✅ | Launch and gameplay perfect |
| SFC | Snes9x | ✅ | Launch and gameplay perfect |
| MD | PicoDrive | ⚠️ | Launches, but some shader/scaling combinations leave Auto CPU near 480 MHz and cause slowdown. Stock shader + 3× scale + linear interpolation runs smoothly around 720 MHz; Powersave (~1.1 GHz observed) and Performance (~1.5 GHz) are smooth |
| FBN | FBNeo | ⚠️ | Required BIOS was absent, so game execution is not validated. After the BIOS error, MinArch would not open its menu or exit; volume and brightness shortcuts still responded |
| PS | PCSX-ReARMed | ⚠️ | Every tested game crashed back to NextUI during launch. Tested 2–3 games including Tony Hawk's Pro Skater 2 and a Castlevania title |
| 32X | PicoDrive | ⬜ | — |
| A2600 | Stella 2014 | ⬜ | — |
| A5200 | a5200 | ⬜ | — |
| A7800 | ProSystem | ⬜ | — |
| C128 | VICE x128 | ⬜ | — |
| C64 | VICE x64 | ⬜ | — |
| COLECO | Gearcoleco | ⬜ | — |
| CPC | Caprice32 | ⬜ | — |
| FDS | FCEUmm | ⬜ | — |
| GG | PicoDrive | ⬜ | — |
| LYNX | Handy | ⬜ | — |
| MGBA | mGBA | ⬜ | Alternate GBA core |
| MSX | blueMSX | ⬜ | — |
| NGP | RACE | ⬜ | — |
| NGPC | RACE | ⬜ | — |
| P8 | Fake-08 | ⬜ | — |
| PCE | Mednafen PCE Fast | ⬜ | — |
| PET | VICE xpet | ⬜ | — |
| PKM | PokeMini | ⬜ | — |
| PLUS4 | VICE xplus4 | ⬜ | — |
| PRBOOM | PrBoom | ⬜ | — |
| PUAE | PUAE 2021 | ⬜ | — |
| SEGACD | PicoDrive | ⬜ | — |
| SG1000 | PicoDrive | ⬜ | — |
| SGB | mGBA | ⬜ | — |
| SMS | PicoDrive | ⬜ | — |
| SUPA | Supafaust | ⬜ | Alternate SNES core |
| VB | Mednafen VB | ⬜ | — |
| VIC | VICE xvic | ⬜ | — |

## Open investigations

1. **PS1 launch crash:** collect `minarch.txt`, core stderr, ROM format, BIOS presence,
   and the last log line before return to NextUI.
2. **FBNeo error recovery:** retest with the required BIOS, then separately reproduce
   the missing-BIOS path and determine why MinArch still processes system shortcuts but
   not MENU/exit.
3. **MD Auto CPU behavior:** reproduce with exact shader/scale/interpolation combinations
   while logging `scaling_cur_freq`, governor, min/max frequency, frame time, and audio
   underruns. Manual governor switching itself is working.
4. Expand the table across RG40XXV, RG28XX, RGcubexx, and other RG XX variants during
   alpha testing, recording only device-specific differences once a core is known-good.
