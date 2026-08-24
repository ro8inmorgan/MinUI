#!/usr/bin/env python3
import argparse
import os
import stat
import zipfile
from datetime import datetime, timezone
from pathlib import Path


def zip_time(epoch: int) -> tuple[int, int, int, int, int, int]:
    return datetime.fromtimestamp(max(epoch, 315532800), timezone.utc).timetuple()[:6]


def add(
    archive: zipfile.ZipFile,
    source: Path,
    name: str,
    timestamp: tuple[int, int, int, int, int, int],
) -> None:
    info = zipfile.ZipInfo(name, timestamp)
    mode = source.lstat().st_mode
    permissions = (
        0o777
        if source.is_symlink()
        else 0o755
        if source.is_dir() or mode & 0o111
        else 0o644
    )
    info.external_attr = (stat.S_IFMT(mode) | permissions) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    if source.is_dir():
        archive.writestr(info, b"")
    elif source.is_symlink():
        archive.writestr(info, os.readlink(source).encode())
    else:
        archive.writestr(info, source.read_bytes())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strip-root", action="store_true")
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", nargs="+", type=Path)
    args = parser.parse_args()
    try:
        timestamp = zip_time(int(os.environ["SOURCE_DATE_EPOCH"]))
    except (KeyError, ValueError) as error:
        raise SystemExit("SOURCE_DATE_EPOCH must be an integer") from error
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(
        args.output,
        "w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
        strict_timestamps=True,
    ) as archive:
        for source in sorted(args.inputs, key=lambda item: item.as_posix()):
            paths = (
                sorted(source.rglob("*"), key=lambda item: item.as_posix())
                if args.strip_root
                else [
                    source,
                    *sorted(source.rglob("*"), key=lambda item: item.as_posix()),
                ]
            )
            for path in paths:
                name = (
                    path.relative_to(source).as_posix()
                    if args.strip_root
                    else path.as_posix()
                )
                add(archive, path, name + ("/" if path.is_dir() else ""), timestamp)


if __name__ == "__main__":
    main()
