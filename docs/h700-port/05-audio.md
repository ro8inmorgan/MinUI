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

## Headphone jack — hardware auto-route works; software detection unused

**Hardware (tested):** plugging headphones auto-mutes the speaker and routes audio
to the jack with no NextUI involvement. Functional audio is correct.

**Software:** Brick-style `SetJack` / separate speaker vs headphones volume / HP
volume icon are **not** wired on h700 — `keymon` never monitors a jack event, so
`GetJack()` stays 0 and the volume UI keeps the speaker path. That is cosmetic /
dual-volume polish only; routing does not depend on it. Optional follow-up if a
HP icon or independent HP volume is wanted later. (`jack_state` sysfs was
historically always 0 and was never needed once hardware auto-mute was confirmed.)

## Bluetooth audio — stock-first A2DP enabled

The clean 2026 RG40XXV stock image contains BlueALSA 4.2.0, its ALSA PCM/control
plugins, `/etc/alsa/conf.d/20-bluealsa.conf`, and the system D-Bus policy. A live
startup probe confirmed that `bluealsa -p a2dp-source` acquires `org.bluealsa` and
registers two SBC source endpoints below `/org/bluez/hci0`.

H700 now follows the tg5040 lifecycle with a stock-only implementation:
- `bt_init.sh` starts BlueALSA only after `hci0` and stock BlueZ are ready, waits for
  its D-Bus name, enables native A2DP volume, initializes the remote transport at a
  nonzero volume, and logs useful startup failures
- `/usr/bin/bluealsa`, the ALSA PCM/control plugins, SBC runtime, ALSA configuration,
  and BlueZ all come from the stock firmware; none are bundled or replaced
- settings exposes the existing maximum-sampling-rate control (the H700-specific
  `NO_BT_AUDIO` build gate is removed)
- shared `audiomon` selects the connected device and writes `$HOME/.asoundrc`; MinArch's
  existing device watcher then reopens SDL audio against the stock `bluealsa` PCM. The
  H700 build alone omits the `delay 0` option rejected by its stock BlueALSA plugin;
  tg5040 and tg5050 output is unchanged
- disabling Bluetooth stops BlueALSA before BlueZ; no `bluetoothd`, `bluetoothctl`,
  or other BlueZ component is bundled or replaced

**Earlier validation (RG40XXV):** AirPods 4 ANC pairing, A2DP connection, automatic
internal-speaker muting/restoration, SBC transport, and user-audible game audio passed.
The original silent stream was not a 44.1/48 kHz problem: BlueZ created the AirPods
transport with absolute volume zero. Setting its `MediaTransport1.Volume` to 127 made
the already-running stream audible. The shipped daemon flags now initialize that value
natively. An A2DP PCM publication race was also observed during an artificial daemon
restart while the device remained logically connected; it is not handled by an
H700-only shared-code workaround unless a normal connection flow reproduces it.

**Lifecycle remainder blocked (2026-07-20):** auto reconnect, menu/game and game/game
switching, five suspend/resume cycles, case/disconnect speaker-restore revalidation,
and Settings sample-rate change+persist were never completed. Revalidation is blocked
because BT powers on but scans no devices and cannot pair on **both stock OS and
BaseOS** — a device/firmware environment failure, not a NextUI-only regression.
Resume the full matrix once discovery works again. Firmware without a runnable stock
BlueALSA is logged as controller-only rather than augmented with another userspace
stack; see 09-roadmap.

## Sample rates

`PLAT_pickSampleRate`: tg5040 logic. One H700-specific fix: it must not call
`GetAudioSink()` (shared-memory settings) — minarch calls it before `InitSettings()`
maps the shm, which segfaulted; it now consults only `PLAT_bluetoothConnected()`.
Codec natively supports 48000/44100.
