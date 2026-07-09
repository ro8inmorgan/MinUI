# 05 — Audio

## Playback path (as shipped, tested ✅)

`SND_init` (shared `api.c`) → SDL audio → **ALSA backend → card 0 `audiocodec`**.
`SDL_AUDIODRIVER=alsa` exported by launch.sh. GBA/SNES/PS1 run full speed with clean
audio, no underruns, on RG40XXV; RG34XXSP behaves the same in user testing.

```
card 0: audiocodec   ← speaker / lineout / headphone (the one we use)
card 1: ahubdam
card 2: ahubhdmi     ← HDMI audio (unused until HDMI out lands)
```

## The dlopen'd-ALSA fix (the port's hardest bug — full story)

Symptom: audio played but was glitchy/"sliced". Cause chain:

1. SDL2 was direct-linked against the tg5040 SDK's `libasound`, which has **no symbol
   versioning**; the SDK's copy of libasound was also bundled at first.
2. At runtime against the device's (versioned) libasound, the dynamic linker resolved
   SDL's *unversioned* references to the **legacy `ALSA_0.9` compatibility symbols** —
   which have value semantics, not the modern pointer-in/out semantics — for the
   whole `snd_pcm_hw_params_set_*` family.
3. `snd_pcm_hw_params_set_rate_near` therefore misbehaved: the codec was clamped to
   **192 kHz** while SDL believed it got **32.768 kHz** → resample math produced
   sliced audio.

Fix (commit `752cefe8`): build SDL2 with `--enable-alsa-shared` so it **dlopens the
device's own libasound** — `dlsym` always resolves the default (current) symbol
version. The bundled libasound copy was removed. Only `libtinyalsa` (used directly by
libmsettings) is bundled. Lesson generalized in 01: prefer dlopen over cross-linking
for libraries that exist on the target.

## Volume / mute (libmsettings — details in 03)

- Master: `digital volume`, a 0–63 **attenuator with a reversed scale** — code writes
  `100 - val` percent. Confirmed correct by listening; the control's TLV metadata is
  garbage, so don't trust `amixer` ranges here. `lineout volume` secondary.
- Mute: `SPK` switch off + saved/restored volume (H700 has no
  `/sys/class/speaker/mute`). Mute toggle enabled for h700 in settings. Tested ✅.
- Validation: no known issues on RG40XXV or RG34XXSP. The volume UI behaves correctly,
  and audible levels are as expected across the full range from mute through 100%.
- Suspend: the `suspend` script saves the full mixer state (`alsactl store`) in
  `before()` and **restores it in `after()`** on resume (an early version had the
  restore commented out; it's live now).

## Headphone jack — not wired (open)

`snd_soc_sunxi_component_jack/parameters/jack_state` exists but was historically
"always 0"; never re-tested on 2026 firmware. Unknown how the stock OS switches
speaker/HP (possibly hardware auto-mute, in which case nothing is needed). To
investigate: diff `amixer contents` and watch input devices while plugging headphones
on a live device. Until then: no jack-based switching in NextUI.

## Bluetooth audio — deliberately disabled this beta

Plan was to build and ship `bluez-alsa` (Ubuntu 22.04 doesn't include it). **Shipped
decision: gate BT audio off instead**:
- settings built with `-DNO_BT_AUDIO` for h700 → BT samplerate menu hidden
  (`btmenu.cpp` made null-safe for the missing item)
- `audiomon` refuses A2DP sinks unless a `bluealsa` binary exists (it doesn't)
- `bt_init.sh` still starts bluealsa *if present*, so dropping a built bluealsa into
  `.system/h700/bin` lights the path up again
- `skeleton/BASE/README.txt` tells users BT audio is off in this beta

BT controller input still works through SDL (07). Shipping bluealsa is the path to
re-enable audio — see 09-roadmap.

## Sample rates

`PLAT_pickSampleRate`: tg5040 logic. One H700-specific fix: it must not call
`GetAudioSink()` (shared-memory settings) — minarch calls it before `InitSettings()`
maps the shm, which segfaulted; it now consults only `PLAT_bluetoothConnected()`.
Codec natively supports 48000/44100.
