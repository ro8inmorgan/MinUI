# 02 — Boot Hijack, Installer & SD Layout

Reference for the stock boot chain, takeover mechanism, storage policy, installer,
and runtime launcher. Current tests live in [08](08-testing-status.md); lifecycle
gates live in [09](09-roadmap.md).

## How NextUI takes over the stock OS (no reflash, fully reversible)

The stock launcher wrapper `/mnt/vendor/ctrl/dmenu_ln` prefers **`/mnt/mmc/dmenu.bin`**
(the FAT32 ROMs partition of the boot SD, TF1) over the built-in frontend (verified,
full chain in 00). Placing our own `dmenu.bin` there hijacks boot; deleting it restores
pure stock. This shipped as designed and is the entire install/uninstall story on TF1.
The rollback invariant is simple: when that file is absent, `dmenu_ln` cannot select
NextUI. BOOT-04 records the corresponding design inspection; 09 owns its gate policy.

```
stock boot:  systemd launcher.service → launcher.sh → loadapp.sh → dmenu_ln
                                                                    │
                              /mnt/mmc/dmenu.bin exists? ───────────┤
                                YES → run it  (= NextUI boot shim)  │
                                NO  → /mnt/vendor/bin/dmenu.bin (stock UI)
```

### Stock “MU style” theme caveat (install prerequisite)
The real hijack break is a **stock UI theme setting**, not a separate muOS install.
In stock (and stockmod) Settings, **MU style 1 / MU style 2** causes `dmenu_ln` to
prefer the MU frontend (`muos1.bin` / `muos2.bin`, gated by `/mnt/vendor/muos*.ini`)
**before** `/mnt/mmc/dmenu.bin` ever runs. Result on hardware: the device boots the
stock/MU UI normally; NextUI never starts; no splash or warning appears — our shim
is simply not executed.

There is nothing useful to do from inside NextUI for that path. The install docs
already state the requirement (`skeleton/BASE/README.txt`): set the stock theme to
**“old style” (default)**, not MU style. The maintained decision is to document this
as an install prerequisite; a warn-or-hard-stop path cannot run before the override.

Note: `boot/boot.sh` still has a `muos1.ini`/`muos2.ini` → “STOCK TARGET REQUIRED”
check (and `launch.sh` can drop `stockmod-warning.txt`), but that path is only
reachable if our `dmenu.bin` was already selected — which the same override prevents.
Treat the splash as defensive dead code unless stock’s `dmenu_ln` logic changes.

## SD card layout: TF1 stays stock, NextUI lives on TF2

**Policy (project owner), implemented as designed:**
- TF1's only write, ever, is the single `dmenu.bin` file on its user-visible FAT
  partition. Stock partitions p1–p7 are never written — no bootlogo replacement, no DTB
  Panel-Fix writes, no apt installs. Both writers of that file (`install/update.sh` and
  the launch.sh self-heal) guard with `mountpoint -q /mnt/mmc` **and** `cmp -s` so they
  only touch TF1 when it's genuinely mounted and the content actually differs.
  - **One sanctioned exception**: the optional `Bootlogo.pak` tool (EXTRAS, see 03)
    writes `bootlogo.bmp` on the vfat boot-resource partition `mmcblk0p2` — the same
    mechanism the old port used at install time, but here only on an explicit user
    action inside the pak, never automatically. Before the first overwrite it copies
    the stock logo into the pak's preset folder as `original.bmp`, so the stock look
    is always restorable from within the pak. The installer/boot shim remain
    write-free on p1–p7.
- **TF2 = NextUI's card** (`/dev/mmcblk1p1`): `.system`, `.tmp_update`, `.userdata`,
  Bios, Roms, Saves. FAT32 recommended; the shim also tries exfat and `-t auto` as
  fallbacks (kernel 4.9 has no native exfat — exfat works only if the stock OS has a
  helper). `SDCARD_PATH` stays `/mnt/SDCARD` via symlink/bind to the real mountpoint,
  so shared code's hardcoded paths keep working.
- **TF1-only layout: not supported** (deliberate scope cut vs the old port — keeps the
  stock card pristine and the support matrix small). No TF2 → "INSERT NEXTUI TF2 CARD"
  splash, then fall through to the stock frontend so the device always boots something.

**End-user install** (documented in `skeleton/BASE/README.txt`):
1. Drag `h700/dmenu.bin` from the release onto TF1's ROMs partition root (any PC + card
   reader).
2. Copy `MinUI.zip` (plus Bios/Roms) onto a FAT32 TF2. First boot self-installs.
3. Uninstall: delete `dmenu.bin` from TF1 → pure stock boot. `.system`/`.userdata` on
   TF2 remain inert data.

## The boot shim (`workspace/h700/boot/`)

`build.sh` produces `dmenu.bin` as a self-extracting script: `boot.sh` + an
`echo BINARY` sentinel + a gzipped tar payload (`fbsplash`, static `unzip` helper).
Notable implementation details (`boot/boot.sh`):

- `trap '' USR1` — the stock `launcher.sh stop` sends SIGUSR1; must not kill the shim.
- Payload offset found with `grep -na '^BINARY' "$0" | cut -d: -f1 | head -1`
  (`head`, not `tail` — the script's own grep line would otherwise match last).
- Logs to `/tmp/nextui-h700.log` by default; only writes a log to TF1 if it is a real
  mountpoint, truncating per boot (no unbounded growth on the stock card).
- **Splash**: `fbsplash` (`boot/fbsplash.c`, ~180 lines) mmaps `/dev/fb0` and renders
  text with a built-in 5×7 bitmap font. It is statically bundled and safe for early
  boot; the shim extracts the embedded payload before invoking it. Messages:
  `INSTALLING NEXTUI`, `UPDATING NEXTUI`,
  `INSERT NEXTUI TF2 CARD`, `NEXTUI INSTALL MISSING`, `STOCK TARGET REQUIRED`.
  (The old port's per-panel raw-BMP `dd` scheme was dropped — rendered text needs no
  per-resolution assets. Post-install, SDL-based `show2.elf` takes over as usual.)
- **Shared helpers**: `shim-common.sh` is packed into the self-extracting payload and
  sourced after extraction, so boot and installer share TF2 mount/repair, `/mnt/SDCARD`
  compatibility, and logging helpers.
- **TF2 mount**: tries vfat → exfat → auto; on failure runs `repair_tf2()` —
  `fsck.fat -a` or `fsck.exfat -a` chosen by `blkid` — then retries once.
- **Update trigger**: `MinUI.zip` always enters the update path. If
  `.tmp_update/h700.sh` is missing (fresh card), the shim bootstrap-extracts only
  `.tmp_update/*` from that zip first, then delegates — **the zip is left in place for
  the installer to own**. Root-level `*.pakz` files also invoke the installer when an
  executable `.tmp_update/h700.sh` is already present; a pakz alone cannot bootstrap
  a fresh card.
- Then `exec`s `.system/h700/paks/MinUI.pak/launch.sh`; any failure falls back to the
  stock frontend.
- Prefers the stock OS's `unzip` when present; the embedded static helper is the
  fallback (stock Ubuntu has a full userland — use it).

## The installer (`workspace/h700/install/`)

- `boot.sh` → ships as `.tmp_update/h700.sh`. Owns the update transaction:
  clean-replaces `.system/h700/bin`, `lib`, and `paks/MinUI.pak` (`rm -rf` then
  extract) so stale files can't survive an update, processes `*.pakz` (including
  `post_install.sh` hooks), and deletes `MinUI.zip` itself when done.
- `update.sh` → ships as `.system/h700/bin/install.sh` (in-place updates from within
  the running OS). Also performs the TF1 `dmenu.bin` self-heal (mountpoint + cmp
  guarded, above).

## `MinUI.pak/launch.sh` (the master runtime script)

Responsibilities as shipped (`skeleton/SYSTEM/h700/paks/MinUI.pak/launch.sh`, ~225 lines):
- `export PLATFORM=h700`, path exports, `/mnt/SDCARD` compat symlink.
- **Device detection**: `RGXX_MODEL=$(strings /mnt/vendor/bin/dmenu.bin | grep -m1 ^RG)`
  → `DEVICE` case (rg28xx / rg34xx / cube / rg40xx default). Confirmed working on 2026
  firmware.
- `LD_LIBRARY_PATH=$SYSTEM_PATH/lib:...` — our SDL2 first.
- Env for the graphics/audio/input stack: `SDL_VIDEODRIVER=mali`,
  `SDL_AUDIODRIVER=alsa`, `SDL_JOYSTICK_DISABLE_UDEV=1`, and `SDL_ROTATION=1` when
  `DEVICE=rg28xx`.
- Kills/stops stock services we replace: `brightCtrl.bin`, `cexpert`, NetworkManager;
  writes a `/run/systemd/logind.conf.d/nextui-h700.conf` drop-in with
  `HandlePowerKey=ignore` + `HandlePowerKeyLongPress=ignore` and restarts logind (so
  systemd never races us on the power button).
- Spawns `keymon.elf`, `batmon.elf`, `audiomon.elf`; wifi/bt init per settings (07);
  runs boot hooks; then the standard crash-restart loop around `nextui.elf` /
  `/tmp/next` chaining.
- Poweroff/reboot via sentinel files: `/tmp/poweroff` → `poweroff`, `/tmp/reboot` →
  `reboot` (systemd handles clean unmounts — works; no `poweroff_next` port needed).
- Crash-loop handling: after 5 consecutive `nextui.elf` crashes, the loop logs the
  crash limit, removes `/tmp/nextui_exec`, and falls through to poweroff. H700 does
  not ship an automatic WiFi/SSH rescue path in the release runtime.

## Maintainer development loop

- The stock OS runs sshd with a full Ubuntu userland. After the first install, copy
  freshly built `.elf` files into `.system/h700/bin` and restart the launch loop;
  routine iteration does not require moving SD cards.
- The bounded crash loop writes its diagnostics before powering off. Retrieve logs
  from TF2, or use stock SSH while the device is already reachable.
- Test the self-extracting shim and installer as separate layers: the shim owns early
  mount/repair/fallback, while `.tmp_update/h700.sh` owns the update transaction.
