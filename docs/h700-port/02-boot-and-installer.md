# 02 — Boot Hijack, Installer & SD Layout

## How NextUI takes over the stock OS (no reflash, fully reversible)

Verified on the live RG40XXV: the stock launcher wrapper `/mnt/vendor/ctrl/dmenu_ln`
prefers **`/mnt/mmc/dmenu.bin`** (the FAT32 ROMs partition of the *boot* SD, TF1)
over the built-in frontend. Placing our own `dmenu.bin` there hijacks boot. Removal =
delete one file. This is the same mechanism the old MinUI rg35xxplus port used.

```
stock boot:  systemd launcher.service → launcher.sh → loadapp.sh → dmenu_ln
                                                                    │
                              /mnt/mmc/dmenu.bin exists? ───────────┤
                                YES → run it  (= NextUI boot shim)  │
                                NO  → /mnt/vendor/bin/dmenu.bin (stock UI)
```

### stockmod caveat (must be in README/install docs)
`dmenu_ln` checks `/mnt/vendor/muos1.ini` / `muos2.ini` **first** and runs the bundled
muOS instead if present (the user's RG40XXV is in this state). Users running stockmod
must switch the boot target back to "stock" (or the installer, once running via any
one-shot exec path, deletes `/mnt/vendor/muos*.ini`). Document prominently:
**NextUI installs on top of the *stock* boot path.**

## SD card layout: stock card gets one drag-dropped file; NextUI lives on TF2

**Policy (project owner):** TF1 remains the stock OS card. NextUI runs from TF2.
Minimal, end-user-simple changes to the stock card are fine (drag-drop a file from a
PC). When in doubt, do what the old rg35xxplus port did — and this is exactly that:

**End-user install (matches old MinUI rg35xxplus flow):**
1. Put TF1 (stock card) in a card reader — the ROMs partition (mmcblk0p8, FAT32) is
   the one a PC sees. **Drag `dmenu.bin` onto its root.** That's the entire stock-card
   change (`dmenu_ln` on the stock OS runs `/mnt/mmc/dmenu.bin` if present — verified;
   it never consults TF2 for a boot target, so this one file is both necessary and
   sufficient for autoboot).
2. Format TF2 FAT32, copy the NextUI base-zip contents onto it (`MinUI.zip`, Bios,
   Roms, …). First boot self-installs to `.system/` — same as every MinUI/NextUI
   platform.
3. Uninstall: delete `dmenu.bin` from TF1 → device boots pure stock again.

Rules that keep the stock card safe:
- Stock OS **partitions p1–p7 (boot-resource, rootfs, vendor, data) are never
  written.** Unlike the old port: no `bootlogo.bmp` replacement on `/dev/mmcblk0p2`,
  no DTB "Panel Fix" writes to `/dev/mmcblk0` (drop that tool initially), no apt
  installs into rootfs (2026 stock already runs sshd). The only TF1 write ever is the
  `dmenu.bin` file on the user-visible FAT partition.
- **TF2 = NextUI's card** (`/dev/mmcblk1p1` → `/mnt/sdcard`): `.system`, `.tmp_update`,
  `.userdata`, Bios, Roms, Saves — everything. FAT32 initially (kernel 4.9 has no
  native exfat; check stock for a FUSE exfat helper before allowing exfat).
  `SDCARD_PATH` stays `/mnt/SDCARD` via symlink → `/mnt/sdcard` (see 03).
- **TF1-only layout: not supported** (old port allowed it; we don't — keeps the stock
  card pristine and the support matrix small). If TF2 is absent, the shim shows an
  "insert NextUI SD card" splash and exec's the stock frontend, so the device always
  boots something sensible.
- dmenu.bin self-heal/update: launch.sh compares `.system/h700/dat/dmenu.bin` (TF2)
  against `/mnt/mmc/dmenu.bin` and re-copies on mismatch — updates ship through TF2
  releases; the user never touches TF1 again after install.
- Verify on current firmware whether stock automounts TF2 at `/mnt/sdcard` before
  `dmenu_ln` runs (mount point exists; card wasn't inserted during probing). If not,
  the shim mounts it — the old port's boot.sh has the exact mount logic to reuse.

## `workspace/h700/boot/` — the dmenu.bin shim

Reuse the old self-extracting design (`git show 8cd78866:workspace/rg35xxplus/boot/build.sh`):
`dmenu.bin` = shell script + `echo BINARY` sentinel + gzip payload (splash images,
static `unzip`). At boot it:

1. Mounts TF2 if the stock OS hasn't already; if TF2 absent → splash "insert NextUI SD card", exec stock frontend (no TF1 fallback, see policy above)
2. Shows splash: `dd` a raw BMP/fb dump to `/dev/fb0` (640×480 RGB565/XRGB — regenerate
   splash assets per panel: default 640×480, `-w` 720×480 for RG34xx, `-r` rotated
   480×640 for RG28XX, chosen by `cat /sys/class/graphics/fb0/modes`)
3. First boot / update: if `MinUI.zip` (or `*.pakz`) present at SD root → run
   `.tmp_update/h700.sh` (self-extracted static unzip available) which unzips into
   `.system/` and `.tmp_update/`
4. `exec .system/h700/paks/MinUI.pak/launch.sh`

Improvement over the old port: current NextUI has `show2.elf --mode=daemon` with
progress display — after first install, prefer it over raw `dd` splash (needs SDL —
only usable post-extract; keep `dd` for the very first boot).

The stock wrapper loops and re-runs `dmenu.bin` when it exits, and `launcher.sh stop`
sends `SIGUSR1` — the shim and launch.sh must not treat either as an error
(NextUI's own `while` launch loop in MinUI.pak handles restarts; on poweroff request
touch `/tmp/poweroff` and call `poweroff` — systemd handles clean shutdown on this OS).

## `workspace/h700/install/`

- `boot.sh` → becomes `.tmp_update/h700.sh`. Model on tg5040's (`workspace/tg5040/install/boot.sh`):
  set governor performance during install, show splash, unzip `MinUI.zip` payload,
  process `*.pakz`, then reboot. Remove all trimui-isms (`/usr/trimui`…).
- `update.sh` → becomes `.system/h700/bin/install.sh` (in-place updates from the OS).
- Re-copy `dmenu.bin` from `.system/h700/dat/dmenu.bin` (TF2) to `/mnt/mmc/dmenu.bin`
  **only when contents differ** (self-healing + updates ship new shims, while keeping
  TF1 writes to the absolute minimum).

## `skeleton/` additions

```
skeleton/SYSTEM/h700/
  bin/            governor.sh, suspend, install.sh, run_hooks.sh, setterm, shutdown-helper
  etc/wifi/       wifi_init.sh          (see 07)
  etc/bluetooth/  bt_init.sh            (see 07)
  paks/MinUI.pak/launch.sh              (master boot script — see below)
  paks/Emus/{FC,GB,GBA,GBC,MD,PS,SFC,…}.pak/   (clone tg5040 set; default.cfg per-device variants)
  shaders/        (copy of tg5040 set)
  system.cfg
skeleton/EXTRAS/Tools/h700/             (Files.pak, Input.pak, Clock.pak, … clone from tg5040 where portable)
```

### MinUI.pak/launch.sh responsibilities (H700 edition)
Clone tg5040's and adapt:
- `export PLATFORM=h700`, standard path exports (SDCARD_PATH=/mnt/sdcard, …)
- **Device detection** (replaces tg5040's `TRIMUI_MODEL` sniff):
  ```sh
  export RGXX_MODEL=$(strings /mnt/vendor/bin/dmenu.bin | grep -m1 ^RG)   # e.g. RG40xxV / RG34xxSP / RG28xx / RGcubexx
  case "$RGXX_MODEL" in
    RG28xx)   export DEVICE="rg28xx" ;;     # rotated panel
    RG34xx*)  export DEVICE="rg34xx" ;;     # 720x480 (+SP has lid)
    RGcubexx) export DEVICE="cube" ;;       # 720x720 (later)
    *)        export DEVICE="rg40xx" ;;     # 640x480 family default
  esac
  ```
  (validate on 2026 firmware; fallback detectors: fb0 mode `480x640` → 28xx,
  `xres/yres` in /sys/class/disp, `axp2202-battery/display_id`, DTB lcd timings)
- `export LD_LIBRARY_PATH=$SYSTEM_PATH/lib:/usr/lib` — **our SDL2 first**
- Stop stock services we replace (crucial for input/audio/sleep hygiene):
  `systemctl stop brightCtrl` isn't a unit — `killall brightCtrl.bin cexpert` instead;
  consider `systemctl stop NetworkManager` if we drive wpa_supplicant directly (see 07)
- governor performance, start `keymon.elf &`, `batmon.elf &`, `audiomon.elf` (if kept), wifi/bt init, boot hooks, then the standard `while` loop around `nextui.elf` / `minarch.elf` (`/tmp/next` mechanics identical to tg5040)
- On loop exit: `/tmp/poweroff` → `poweroff`; `/tmp/reboot` → `reboot` (systemd versions; no custom poweroff_next I2C dance needed — **verify `poweroff` works cleanly from our context**, else port `poweroff_next`)

## Uninstall story (document it)
Delete `dmenu.bin` from the ROMs partition (visible from any PC via card reader) →
stock boots untouched. `.system`/`.userdata` remain inert data.
