# 05 — Audio

## Playback path (shared code, low risk)
`workspace/all/common/api.c` `SND_init` uses SDL audio (`SDL_OpenAudioDevice`) with a
software resampler. Our custom SDL2 is built with the **ALSA** backend → default ALSA
device on card 0 (`audiocodec`). Verified on device:

```
card 0: audiocodec   ← speaker / lineout / headphone
card 1: ahubdam
card 2: ahubhdmi     ← HDMI audio (phase 3, together with HDMI video)
```

Work items:
- Ensure a sane default ALSA route: launch.sh runs an `alsactl restore`-style init or
  explicit `amixer` unmute sequence (`SPK Switch` on, `LINEOUT Switch`, DAC mixers on —
  capture a known-good `alsa.state` from the stock OS while its UI plays sound, ship it).
- `SDL_AUDIODRIVER=alsa` exported in launch.sh (belt & braces).
- If default-device selection misbehaves, set `AUDIODEV=hw:0,0` / configure
  `/etc/asound.conf` in our environment (we can ship one and point `ALSA_CONFIG_PATH`
  at it without touching rootfs).

## Volume / mute (libmsettings, see 03)
- Master: `digital volume` mixer ctl; line/speaker: `lineout volume` (old port drove
  volume with `amixer sset 'lineout volume' N%`; NextUI uses tinyalsa directly —
  same controls, discover exact ranges with `amixer cget` on device).
- Mute: `SPK Switch` off (+ store/restore volume), since tg5040's
  `/sys/class/speaker/mute` doesn't exist here.
- `PLAT_overrideMute` accordingly.

## Headphone jack
- The codec driver module exposes `snd_soc_sunxi_component_jack/parameters/jack_state`;
  the old port found it non-functional ("always 0"). Re-test on 2026 firmware.
- Investigate how the *stock* OS switches speaker/HP (it does): watch
  `amixer contents` diff and kernel log while plugging headphones on the live device;
  there may be an ALSA jack kctl or an input switch event (SW_HEADPHONE_INSERT on some
  BSPs). Wire whatever exists into `audiomon`/`PLAT_audioDeviceWatch*`; worst case,
  speaker stays on lineout auto-switch in hardware (many Anbernic units mute the
  speaker in hardware when jack inserted — if so, we need do nothing).

## Bluetooth audio
See 07 — BlueALSA vs PulseAudio decision. NextUI's generic_bt streams via bluealsa +
`PLAT_pickSampleRate` limits. Ubuntu 22.04 has BlueZ 5.64 (modern), and bluealsa is
buildable; keep parity with tg5040's approach to reuse generic_bt.c unchanged.

## Sample rates
`PLAT_pickSampleRate(requested, max)`: copy tg5040 logic (clamp; when BT connected,
clamp to configured BT limit). The sunxi codec supports 48000/44100 natively — no
platform quirk expected.
