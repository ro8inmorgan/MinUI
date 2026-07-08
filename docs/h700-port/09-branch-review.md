# 09 — h700 Branch Review (2026-07-08, branch tip `d8685fff`)

Review of the `h700` branch against the plan docs and live-probed device facts.
**Verdict: architecturally faithful to the plan and remarkably complete for a first
beta — toolchain reuse, custom SDL2, cores, skeleton, suspend, installer are all
present and mostly correct. A handful of concrete bugs need fixing before/at first
on-device boot; three need a physical device test to settle.**

## A. Confirmed bugs (verified against live device probes)

1. **GPU thermal zone wrong** — [platform.c:143](../../workspace/h700/platform/platform.c):
   `PLAT_getGPUTemp` reads `thermal_zone2`. Probed on RG40XXV: zone0=cpu, **zone1=gpu**,
   zone2=**ve** (video engine), zone3=ddr, zone4=axp2202-battery. Fix: zone1.
   (`PLAT_getCPUTemp` zone0 is correct.)

2. **SDL joystick button indices likely wrong** — [platform.h:92-100](../../workspace/h700/platform/platform.h).
   evtest on the live device shows event1 exposes exactly KEY codes
   {1, 114, 115, 304–316, 354} — **no d-pad keycodes** (d-pad = ABS hats 16/17).
   SDL's linux joystick driver assigns button indices scanning BTN-range codes first
   (304→0 … 316→12, 354→13), then low codes (1→14, 114→15, 115→16). Therefore:
   - `JOY_PLUS=18 / JOY_MINUS=17` cannot be right → predicted **PLUS(115)=16, MINUS(114)=15**
   - Branch maps L2=9(=code 313), R2=10(=314), L3=11(=315), R3=12(=316). The old
     *working* port's wiring was **L3=313, L2=314, R2=315, R3=316** → predicted SDL
     indices **L3=9, L2=10, R2=11, R3=12**. Branch and old port disagree; one 2-minute
     `jstest`/`evtest` session (press L2, L3, R2, R3, Vol+/- and note codes) settles it.
     The branch already builds `jstest` + `evtest` in `workspace/h700/other/` — use them.
   - Everything else verified consistent: A=0 B=1 Y=2 X=3 L1=4 R1=5 SELECT=6 START=7
     MENU=8 (per old-port wiring + SDL enumeration), axes LX=0 LY=1 RX=2 RY=3
     (ABS_Z/RX/RY/RZ, signed ±4096, flat=32 — probed).

3. **`PLAT_shouldWake` can drop the wake press** — [platform.c:265-286](../../workspace/h700/platform/platform.c):
   it opens `/dev/input/event0` fresh on every poll and closes it again. evdev only
   queues events for *open* fds — a power press+release that lands between polls is
   lost entirely, making wake-from-sleep flaky. Fix: open once (static fd, like the
   old port's persistent `input_fds`), drain per poll, close in a quit path.

## B. Must-verify on first device boot (can't be settled from the desk)

4. **`--disable-loadso` in the SDL2 configure** — [workspace/h700/makefile:26](../../workspace/h700/makefile).
   SDL's EGL glue (`SDL_egl.c`) normally dlopens libEGL via SDL_LoadObject; with
   loadso disabled, `SDL_GL_CreateContext` in the mali driver may fail outright
   ("SDL is compiled without loadso" class errors). It's also *why* libasound now
   needs bundling (ALSA gets linked directly instead of dlopened). If the first GL
   test fails there, drop `--disable-loadso` (and `--disable-filesystem` while at it
   — SDL_GetPrefPath users). NextUI's own GL calls are direct-linked
   (`-lGLESv2`, `GL_GLEXT_PROTOTYPES`) and unaffected; only SDL's context creation
   path is at risk.

5. **`digital volume` direction** — [msettings.c:~1240](../../workspace/h700/libmsettings/msettings.c)
   uses `set_percent(digital, 100 - val)` (inverted). Probed: range 0–63; the ALSA TLV
   metadata (`dBscale-min=0.00dB` at 0, mute=1) suggests 0=mute/quietest — i.e. NOT
   inverted — but sunxi `digital volume` is traditionally an attenuator (0=loudest)
   and the TLV data looks garbage (step 654dB). 30-second listen test decides; if
   inverted is wrong, max volume currently produces silence.

6. **Bundled `libasound.so` shadowing** — commit `d8685fff` ships tg5040's libasound
   into `.system/h700/lib`, which shadows Ubuntu's own (LD_LIBRARY_PATH puts our lib
   dir first). The device HAS libasound (alsa-utils work); the tg5040 copy's
   compiled-in config path may or may not resolve on Ubuntu. If audio misbehaves,
   *delete the bundled libasound* rather than debugging it — the system one is fine
   and SDL's direct link only needs the soname. Similarly `libUMP` is an old
   Utgard-era lib; Mali-G31/kbase doesn't use UMP — likely shippable dead weight
   (harmless, but the unguarded `cp $(PREFIX)/lib/libUMP.so*` fails the build if the
   sysroot ever drops it).

## C. Functional gaps vs plan (from the build/scripts sweep)

7. **Update flow loses the clean-replace step & pakz-only installs**
   ([boot.sh:81-101](../../workspace/h700/boot/boot.sh), [install/boot.sh:52-54](../../workspace/h700/install/boot.sh)):
   boot.sh unzips MinUI.zip itself and **deletes the zip before running
   `.tmp_update/h700.sh`**, so h700.sh's `rm -rf` of bin/lib/MinUI.pak never fires →
   stale files accumulate across updates. And the whole install block only triggers
   on MinUI.zip, so `*.pakz` dropped at the SD root (Pak Store flow) are never
   processed. Fix: make boot.sh delegate — if `.tmp_update/h700.sh` exists, run it
   with the zip still in place (tg5040 model: the installer owns extraction);
   self-extract only on true first install; add a `*.pakz` existence trigger.

8. **WiFi credentials don't survive reboot** —
   [wifi_init.sh:4-6](../../skeleton/SYSTEM/h700/etc/wifi/wifi_init.sh) puts
   `wpa_supplicant.conf` in `/tmp/wifi/` (tmpfs); `generic_wifi.c` persists networks
   via `wpa_cli save_config` → written to tmpfs → gone at poweroff. Point `-c` at
   e.g. `$SDCARD_PATH/.userdata/h700/wpa_supplicant.conf` (keep ctrl sockets in
   `/tmp/wifi/sockets` — that pairing with platform.c's `WIFI_SOCK_DIR` is correct).
   Also `killall wpa_supplicant` should be `systemctl stop wpa_supplicant` (stock
   runs it as a systemd unit that may restart).

9. **BT audio can't work yet: bluealsa neither built nor shipped.**
   `bt_init.sh` starts it only `if command -v bluealsa` — silently never true on
   Ubuntu 22.04. Pairing/input will work (BlueZ 5.64 + btagent ✔); A2DP needs
   bluez-alsa built in the toolchain and shipped in `.system/h700/{bin,lib}` (plan 07).
   Decide: ship it, or gate BT-audio UI off for the first beta.

10. **TF1-write guard ineffective** — [install/update.sh:15](../../workspace/h700/install/update.sh),
    launch.sh dmenu refresh: guarded by `[ -d /mnt/mmc ]`, which is true even when
    nothing is mounted there → would write dmenu.bin onto the stock **rootfs**,
    violating the p1–p7 policy. Use `mountpoint -q /mnt/mmc`.

11. **No first-boot splash** — boot.sh extraction (potentially ~1 min on FAT) runs on
    a dead screen; plan 02 calls for a raw-fb `dd` splash + an "insert NextUI SD
    card" image when TF2 is missing. show2 only appears once h700.sh runs.

12. **DHCP re-association** — `dhclient -nw` is started once; switching networks later
    via wpa_cli may not re-DISCOVER. Verify; if flaky, add a `wpa_cli -a` action
    script that bounces dhclient on CONNECTED.

## D. Leftovers & cleanup

13. `skeleton/SYSTEM/h700/bin/reboot_next` is a byte-identical tg5040 copy (busybox
    reboot, `/etc/profile` umount hack). It's dead code on h700 — delete it (launch.sh
    correctly uses plain `reboot`/`poweroff`).
14. Dead `-brick` cfgs can never load (minarch selects `default-$DEVICE.cfg`; DEVICE ∈
    rg40xx/rg34xx/rg28xx/cube): `system-brick.cfg`, `paks/Emus/{FC,PS,SFC}.pak/default-brick.cfg`,
    `EXTRAS/Emus/h700/{CPC,PUAE,FBN,VB}.pak/default-brick.cfg`. Delete. (Conversely
    rg34xx/rg28xx/cube have no device cfgs yet — fine for rg40xx-first, note for later.)
15. NextCommander is built with `PLATFORM=tg5040` → UI metrics tuned for 1280×720; on
    640×480 Files.pak will render oversized. Needs an h700 stanza upstream (the
    `FILE_SYSTEM=/dev/mmcblk1p1` it inherits is coincidentally correct).
16. SDL clone is unpinned (`--depth 1` default branch) — pin a commit hash; and assert
    `SDL_VIDEO_DRIVER_MALI 1` in the generated config after configure, else a failed
    mali check silently produces a dummy-video SDL that "builds fine".
17. Minor: boot.sh `BINARY` sentinel uses `tail -1` (payload bytes could false-match —
    `head -1` is safer); no `trap '' USR1` in the shim (stock `launcher.sh stop` sends
    SIGUSR1); `LOG_PATH=/mnt/mmc/nextui-h700.log` grows unbounded on TF1 (truncate per
    boot / move to TF2); `make PLATFORM=h700 shell` doesn't get the PLATFORM env
    override that build targets got; `PLAT_getGPUSpeed()` hardcodes 660 MHz;
    `PLAT_enableBacklight` doesn't drive `work_led` (plan 06 nicety); `SCREEN_FPS 60.0`
    still needs per-panel measurement; settings.elf glib: sysroot version must be ≤
    device's 2.72 (verify once).

## E. Done well (verified)

- **Toolchain reuse** exactly per plan: `TOOLCHAIN_NAME=tg5040` override covers pull/
  clone/init/clean; PLATFORM/UNION_PLATFORM correctly forced into the container
  (`PREFIX_LOCAL=/opt/nextui` is a fixed ENV — no tg5040 leakage); no tg5050/tg5040
  regression (additive filters only).
- **Custom SDL2 built and shipped**: SDL-malifbdev-rot with mali+ALSA only, installed
  to PREFIX_LOCAL so `pkg-config sdl2` resolves to it; `libSDL2*` bundled to
  `.system/h700/lib`; launch.sh sets lib-dir-first LD_LIBRARY_PATH and
  `SDL_VIDEODRIVER=mali`. Matches plan 04.
- **Cores**: full 28-core tg5040 list, all 29 patches properly retargeted, picodrive
  LTO fix correct, stock output names match packaging.
- **launch.sh**: zero trimui-isms; TF2 mount + `/mnt/SDCARD` compat symlink; DEVICE
  detection consistent across launch.sh ↔ platform.c ↔ settings.cpp ("rg40xx"/
  "rg34xx"/"rg28xx"/"cube"); stock daemons killed; `nextui_exec` loop byte-compatible
  with tg5040 conventions; systemd-native poweroff/reboot; debug-keep-network toggle.
- **suspend**: faithful tg5040 port — mem-write retry + false-negative workaround,
  ALSA state save/restore, wifi/bt stop-before/start-after (covers the verified
  "WiFi doesn't auto-recover" issue), hooks, `work_led` sleep indication.
- **governor.sh**: correct policy paths and sane mapping within probed 480–1512 MHz.
- **updater**: device-tree check ordered before the ambiguous `0xd03` case, zero28
  arm guarded — exactly per plan.
- **settings.cpp**: clean Anbernic vendor/model plumbing; colortemp/contrast/exposure/
  displaycal gates extended to h700; displaycal compiled into libmsettings ✔.
- **keymon**: correct codes (MENU 312/354 + SELECT 310 modifiers, vol 114/115),
  repeat + sleep-gap handling, reads event0-2.
- **Lid support** (RG34XXSP) already implemented in platform.c via `hallkey` with
  closed-lid wake gating.

## Suggested fix order

1. A1 (thermal zone), A3 (shouldWake fd), C10 (mountpoint guard), D13/D14 (deletions) — mechanical, do now.
2. C7 (installer flow) + C8 (wpa conf persistence) — before first real install.
3. First on-device boot: settle B4 (loadso), B5 (volume direction), A2 (jstest button map), B6 (libasound), C12 (dhclient).
4. Then: C9 (bluealsa decision), C11 (splash), D15-17 polish.
