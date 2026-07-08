# 10 — h700 Branch Review Supplement (2026-07-08)

Additive findings **not covered** in [09-branch-review.md](09-branch-review.md).
Use doc 09 as the primary review; this file only extends it.

**Primary verdict (doc 09):** architecturally faithful to the plan and remarkably
complete for a first beta. This supplement records code-quality issues, validation
debt, and one factual correction to doc 09 §E.

---

## Completion framing

Doc 09's verdict stands: this is a **faithful, build-complete first beta**, not a
finished port. Against [08-testing-and-rollout.md](08-testing-and-rollout.md):

- Phases 1–6 are largely implemented in tree
- **Phase 8** (full validation matrix) is entirely open
- **Phase 9** (RG28XX rotation) has not started
- No on-device parity testing is recorded yet

---

## F. Additional findings (not in doc 09)

**18. `scaleColortemp()` undefined behavior on bad input** —
[workspace/h700/libmsettings/msettings.c](../../workspace/h700/libmsettings/msettings.c):
the switch covers 0–40 but has no `default`; a corrupt settings value returns an
uninitialized `raw`.

**19. Production debug logging in libmsettings** — `SetRawBrightness`,
`SetRawColortemp`, `SetRawDisplayCal`, `SetJack`, `SetAudioSink`, contrast/
saturation/exposure all `printf` to stdout on every change. Will flood
`$LOGS_PATH/*.txt` on device; strip or gate behind a debug flag before beta.

**20. `PLAT_setDateTime()` buffer overflow risk** —
[platform.c](../../workspace/h700/platform/platform.c) uses `sprintf(cmd, ...)` into
a 512-byte stack buffer; the governor path already uses `snprintf`. Align them.

**21. `PLAT_getModel()` returns `getenv("RGXX_MODEL")` directly** — fragile if
anything retains the pointer across env changes. Low probability at runtime, but a
static copy would be safer.

**22. HDMI hotplug never wired** — `GetHDMI()` / `SetHDMI()` are no-ops in h700
`msettings.c`. [00-device-facts.md](00-device-facts.md) confirms
`/sys/class/extcon/hdmi/` exists; `hdmimon()` in minarch/nextui therefore never
fires. Affects HDMI-aware sleep, rumble skip (plan 03), and any HDMI UI.

**23. `hasMuteToggle()` excludes h700** — [settings.cpp](../../workspace/all/settings/settings.cpp)
gates the FN-switch / mute-advanced UI to tg5040/tg5050 only. H700 has mute via `SPK`
mixer + `PLAT_overrideMute`; users won't see controls the hardware supports.

**24. Post-resume display state not re-applied** — Plan 06 calls for re-applying
brightness + displaycal gamma LUT after wake (panel may clear LUT). No hook in
`suspend`, `launch.sh`, or a `post-resume.d` default script does this.

**25. Suspend `after()` races the UI** —
[suspend](../../skeleton/SYSTEM/h700/bin/suspend) runs service restore (`after &`) in
background for wake latency. UI can resume before WiFi/BT are up — relevant for RA
login, Pak Store, settings WiFi pane immediately after wake.

**26. muOS precedence is warn-only** — [boot.sh](../../workspace/h700/boot/boot.sh)
and [launch.sh](../../skeleton/SYSTEM/h700/paks/MinUI.pak/launch.sh) log/write
`stockmod-warning.txt` when `muos1.ini` is present but still boot NextUI. Plan 02
flags this as a top risk; consider hard-failing or a blocking installer message.

**27. systemd logind power-key conflict unaddressed** — Plan 06 says verify
`HandlePowerKey=ignore`; `launch.sh` doesn't configure logind. Risk of
double-handling with NextUI's power-key sleep/wake.

**28. No TF2 filesystem recovery** — Plan 08 calls for a vfat `fsck.fat` hook
(MinUI.pak e2fsck equivalent). Not present in h700 skeleton.

**29. RG28XX rotation still unimplemented** — Doc 09 D14 notes missing per-device emu
cfgs; worth stating explicitly: `is_rg28xx` is detected but there is no SDL rotation
env, no `PLAT_getDisplayRotation()` hook in `generic_video.c`, and no 480×640 panel
handling beyond logical 640×480.

**30. 480p UI audit not started** — Plan 04's top risk (first 480p platform in ~2
years): fonts, pills, quick switcher at 640×480. No evidence of a desktop or on-device
pass.

**31. No user-facing install docs** — [skeleton/BASE/README.txt](../../skeleton/BASE/README.txt)
has no RG40XX / H700 / `dmenu.bin` section. Install flow exists in code but isn't
documented for end users.

**32. Correction to doc 09 §E ("suspend")** — §E claims "ALSA state save/restore."
The script **saves** in `before()` but **restore is commented out** in `after()`
(lines 58–60). Save-only is accurate; full restore is not implemented.

---

## G. Suggested additions to doc 09 fix order

Doc 09's sequence is good. Slot these in:

| When | Items |
|---|---|
| Before first boot (mechanical) | #18 (`scaleColortemp` default), #19 (strip debug prints) |
| With C7/C8 (installer + WiFi) | #24 (post-resume displaycal/brightness hook) |
| First on-device session | #22 (HDMI extcon probe), #23 (mute UI gate decision), #25 (post-wake WiFi race) |
| Before any public beta | #29–31 (rotation, 480p UI, README) |

Do **not** re-litigate doc 09 items A1–D17 here — they remain the priority queue.