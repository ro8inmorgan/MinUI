# 10 — Core & Game Compatibility Matrix

This is the system-by-system H700 compatibility ledger and the detailed evidence
behind the games and performance rows in [08-testing-status.md](08-testing-status.md#games-and-performance).
It records test coverage, reproducible issues, and device-specific differences;
[09-roadmap.md](09-roadmap.md) decides lifecycle requirements. The first pass is on
RG34XXSP (720×480); add RG40XXV and other RG XX results where behavior differs.

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

## RG34XXSP results (updated 2026-07-20)

| System | Core | Status | Evidence / issue |
|---|---|---|---|
| GB | Gambatte | ✅ | Launch and gameplay perfect |
| GBC | Gambatte | ✅ | Launch and gameplay perfect |
| GBA | gpSP | ✅ | Launch and gameplay perfect |
| FC | FCEUmm | ✅ | Launch and gameplay perfect |
| SFC | Snes9x | ✅ | Launch and gameplay perfect |
| MD | PicoDrive | ✅ | Launch and gameplay good. Former Auto CPU slowdown (some shader/scale combos stuck near 480 MHz) is fixed — user-verified 2026-07-20 |
| FBN | FBNeo | ⚠️ | With required BIOS: launch and play OK (2026-07-20). Without BIOS: MinArch hard-locks (no menu/exit; power cycle required). The upstream FBNeo error path does not poll frontend input. Track upstream. |
| PS | PCSX-ReARMed | ✅ | Launch and gameplay pass after a clean core rebuild. The earlier crashes across 2–3 titles were caused by a stale/corrupted core, not a model-specific runtime defect. H700 `PS.pak/default.cfg` binds L3/R3 for DualShock stick clicks |
| 32X | PicoDrive | ⬜ | — |
| A2600 | Stella 2014 | ✅ | Launch and gameplay pass (2026-07-20) |
| A5200 | a5200 | ⬜ | — |
| A7800 | ProSystem | ⬜ | — |
| C128 | VICE x128 | ⬜ | — |
| C64 | VICE x64 | ⬜ | — |
| COLECO | Gearcoleco | ⬜ | — |
| CPC | Caprice32 | ⬜ | — |
| FDS | FCEUmm | ⬜ | — |
| GG | PicoDrive | ⬜ | — |
| LYNX | Handy | ⬜ | — |
| MGBA | mGBA | ✅ | Alternate GBA core — launch and gameplay pass (2026-07-20) |
| MSX | blueMSX | ⬜ | — |
| NGP | RACE | ⬜ | — |
| NGPC | RACE | ⬜ | — |
| P8 | Fake-08 | ✅ | Launch and gameplay pass (maintainer-tested) |
| PCE | Mednafen PCE Fast | ⬜ | — |
| PET | VICE xpet | ⬜ | — |
| PKM | PokeMini | ⬜ | — |
| PLUS4 | VICE xplus4 | ⬜ | — |
| PRBOOM | PrBoom | ⬜ | — |
| PUAE | PUAE 2021 | ⬜ | — |
| SEGACD | PicoDrive | ⬜ | — |
| SG1000 | PicoDrive | ⬜ | — |
| SGB | mGBA | ⬜ | — |
| SMS | PicoDrive | ✅ | Launch and gameplay pass (2026-07-20) |
| SUPA | Supafaust | ⬜ | Alternate SNES core |
| VB | Mednafen VB | ⬜ | — |
| VIC | VICE xvic | ⬜ | — |
